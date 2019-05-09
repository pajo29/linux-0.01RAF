/*
 *	console.c
 *
 * This module implements the console io functions
 *	'void con_init(void)'
 *	'void con_write(struct tty_queue * queue)'
 * Hopefully this will be a rather complete VT102 implementation.
 *
 */

/*
 *  NOTE!!! We sometimes disable and enable interrupts for a short while
 * (to put a word in video IO), but this will work even for keyboard
 * interrupts. We know interrupts aren't enabled when getting a keyboard
 * interrupt, as we use trap-gates. Hopefully all is well.
 */

#include <linux/sched.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <asm/system.h>

#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>


#define SCREEN_START 0xb8000
#define SCREEN_END   0xc0000
#define LINES 25
#define COLUMNS 80
#define NPAR 16

#define TOOL_COL_START 57
#define TOOL_COL_END 79
#define TOOL_LINE_START 0
#define TOOL_LINE_END 12

extern void keyboard_interrupt(void);

static unsigned long origin=SCREEN_START;
static unsigned long scr_end=SCREEN_START+LINES*COLUMNS*2;
static unsigned long pos;
static unsigned long x,y;
static unsigned long top=0,bottom=LINES;
static unsigned long lines=LINES,columns=COLUMNS;
static unsigned long state=0;
static unsigned long npar,par[NPAR];
static unsigned long ques=0;
static volatile unsigned char special = 0x03;
static unsigned char attr=0x07;

static volatile unsigned char regular = 0x07;
static volatile unsigned char dir = 0x03;
static volatile unsigned char exe_files = 0x02;
static volatile unsigned char dev = 0x06;

static volatile int tool = 0; //0 for off, 1 for dir, 2 for clipboard

volatile int f1_down_flag = 0;

/*
 * this is what the terminal answers to a ESC-Z or csi0c
 * query (= vt100 response).
 */
#define RESPONSE "\033[?1;2c"



static inline void gotoxy(unsigned int new_x,unsigned int new_y)
{
	if (new_x>=columns || new_y>=lines)
		return;
	x=new_x;
	y=new_y;
	pos=origin+((y*columns+x)<<1);
}

static inline void set_origin(void)
{
	cli();
	outb_p(12,0x3d4);
	outb_p(0xff&((origin-SCREEN_START)>>9),0x3d5);
	outb_p(13,0x3d4);
	outb_p(0xff&((origin-SCREEN_START)>>1),0x3d5);
	sti();
}

static void scrup(void)
{
	if (!top && bottom==lines) {
		origin += columns<<1;
		pos += columns<<1;
		scr_end += columns<<1;
		if (scr_end>SCREEN_END) {
			
			int d0,d1,d2,d3;
			__asm__ __volatile("cld\n\t"
				"rep\n\t"
				"movsl\n\t"
				"movl %[columns],%1\n\t"
				"rep\n\t"
				"stosw"
				:"=&a" (d0), "=&c" (d1), "=&D" (d2), "=&S" (d3)
				:"0" (0x0720),
				 "1" ((lines-1)*columns>>1),
				 "2" (SCREEN_START),
				 "3" (origin),
				 [columns] "r" (columns)
				:"memory");

			scr_end -= origin-SCREEN_START;
			pos -= origin-SCREEN_START;
			origin = SCREEN_START;
		} else {
			int d0,d1,d2;
			__asm__ __volatile("cld\n\t"
				"rep\n\t"
				"stosl"
				:"=&a" (d0), "=&c" (d1), "=&D" (d2) 
				:"0" (0x07200720),
				"1" (columns>>1),
				"2" (scr_end-(columns<<1))
				:"memory");
		}
		set_origin();
	} else {
		int d0,d1,d2,d3;
		__asm__ __volatile__("cld\n\t"
			"rep\n\t"
			"movsl\n\t"
			"movl %[columns],%%ecx\n\t"
			"rep\n\t"
			"stosw"
			:"=&a" (d0), "=&c" (d1), "=&D" (d2), "=&S" (d3)
			:"0" (0x0720),
			"1" ((bottom-top-1)*columns>>1),
			"2" (origin+(columns<<1)*top),
			"3" (origin+(columns<<1)*(top+1)),
			[columns] "r" (columns)
			:"memory");
	}
}

static void scrdown(void)
{
	int d0,d1,d2,d3;
	__asm__ __volatile__("std\n\t"
		"rep\n\t"
		"movsl\n\t"
		"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */
		"movl %[columns],%%ecx\n\t"
		"rep\n\t"
		"stosw"
		:"=&a" (d0), "=&c" (d1), "=&D" (d2), "=&S" (d3)
		:"0" (0x0720),
		"1" ((bottom-top-1)*columns>>1),
		"2" (origin+(columns<<1)*bottom-4),
		"3" (origin+(columns<<1)*(bottom-1)-4),
		[columns] "r" (columns)
		:"memory");
}

static void lf(void)
{
	if (y+1<bottom) {
		y++;
		pos += columns<<1;
		return;
	}
	scrup();
}

static void ri(void)
{
	if (y>top) {
		y--;
		pos -= columns<<1;
		return;
	}
	scrdown();
}

static void cr(void)
{
	pos -= x<<1;
	x=0;
}

static void del(void)
{
	if (x) {
		pos -= 2;
		x--;
		*(unsigned short *)pos = 0x0720;
	}
}

static void csi_J(int par)
{
	long count;
	long start;

	switch (par) {
		case 0:	/* erase from cursor to end of display */
			count = (scr_end-pos)>>1;
			start = pos;
			break;
		case 1:	/* erase from start to cursor */
			count = (pos-origin)>>1;
			start = origin;
			break;
		case 2: /* erase whole display */
			count = columns*lines;
			start = origin;
			break;
		default:
			return;
	}
	int d0,d1,d2;
	__asm__ __volatile__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		:"=&c" (d0), "=&D" (d1), "=&a" (d2)
		:"0" (count),"1" (start),"2" (0x0720)
		:"memory");
}

static void csi_K(int par)
{
	long count;
	long start;

	switch (par) {
		case 0:	/* erase from cursor to end of line */
			if (x>=columns)
				return;
			count = columns-x;
			start = pos;
			break;
		case 1:	/* erase from start of line to cursor */
			start = pos - (x<<1);
			count = (x<columns)?x:columns;
			break;
		case 2: /* erase whole line */
			start = pos - (x<<1);
			count = columns;
			break;
		default:
			return;
	}
	int d0,d1,d2;
	__asm__ __volatile__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		:"=&c" (d0), "=&D" (d1), "=&a" (d2)
		:"0" (count),"1" (start),"2" (0x0720)
		:"memory");
}

void csi_m(void)
{
	int i;

	for (i=0;i<=npar;i++)
		switch (par[i]) {
			case 0:attr=0x07;break;
			case 1:attr=0x0f;break;
			case 4:attr=0x0f;break;
			case 7:attr=0x70;break;
			case 27:attr=0x07;break;
		}
}

static inline void set_cursor(void)
{
	cli();
	outb_p(14,0x3d4);
	outb_p(0xff&((pos-SCREEN_START)>>9),0x3d5);
	outb_p(15,0x3d4);
	outb_p(0xff&((pos-SCREEN_START)>>1),0x3d5);
	sti();
}

static void respond(struct tty_struct * tty)
{
	char * p = RESPONSE;

	cli();
	while (*p) {
		PUTCH(*p,tty->read_q);
		p++;
	}
	sti();
	copy_to_cooked(tty);
}

static void insert_char(void)
{
	int i=x;
	unsigned short tmp,old=0x0720;
	unsigned short * p = (unsigned short *) pos;

	while (i++<columns) {
		tmp=*p;
		*p=old;
		old=tmp;
		p++;
	}
}

static void insert_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom=lines;
	scrdown();
	top=oldtop;
	bottom=oldbottom;
}

static void delete_char(void)
{
	int i;
	unsigned short * p = (unsigned short *) pos;

	if (x>=columns)
		return;
	i = x;
	while (++i < columns) {
		*p = *(p+1);
		p++;
	}
	*p=0x0720;
}

static void delete_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom=lines;
	scrup();
	top=oldtop;
	bottom=oldbottom;
}

static void csi_at(int nr)
{
	if (nr>columns)
		nr=columns;
	else if (!nr)
		nr=1;
	while (nr--)
		insert_char();
}

static void csi_L(int nr)
{
	if (nr>lines)
		nr=lines;
	else if (!nr)
		nr=1;
	while (nr--)
		insert_line();
}

static void csi_P(int nr)
{
	if (nr>columns)
		nr=columns;
	else if (!nr)
		nr=1;
	while (nr--)
		delete_char();
}

static void csi_M(int nr)
{
	if (nr>lines)
		nr=lines;
	else if (!nr)
		nr=1;
	while (nr--)
		delete_line();
}

static int saved_x=0;
static int saved_y=0;

static void save_cur(void)
{
	saved_x=x;
	saved_y=y;
}

static void restore_cur(void)
{
	x=saved_x;
	y=saved_y;
	pos=origin+((y*columns+x)<<1);
}



void con_write(struct tty_struct * tty)
{
	int nr;
	char c;
	

	nr = CHARS(tty->write_q);
	while (nr--) {
		GETCH(tty->write_q,c);
		switch(state) {
			case 0:
				if (c>31 && c<127) {
					if (x>=columns) {
						x -= columns;
						pos -= columns<<1;
						lf();
					}
					
					if(c == '#')
					{
					char oldC = c;
					c = '~';
					__asm__(
						"movb attr, %%ah\n\t"
						"movw %%ax, %1\n\t"
						:: "a" (c), "m" (*(short *)pos)
						);
					c = oldC;
					}
					else
					{
					__asm__("movb attr,%%ah\n\t"
						"movw %%ax,%1\n\t"
						::"a" (c),"m" (*(short *)pos)
						/*:"ax"*/);
					}
					pos += 2;
					x++;
				} else if (c==27)
					state=1;
				else if (c==10 || c==11 || c==12)
					lf();
				else if (c==13)
					cr();
				else if (c==ERASE_CHAR(tty))
					del();
				else if (c==8) {
					if (x) {
						x--;
						pos -= 2;
					}
				} else if (c==9) {
					c=8-(x&7);
					x += c;
					pos += c<<1;
					if (x>columns) {
						x -= columns;
						pos -= columns<<1;
						lf();
					}
					c=9;
				}
				break;
			case 1:
				state=0;
				if (c=='[')
					state=2;
				else if (c=='E')
					gotoxy(0,y+1);
				else if (c=='M')
					ri();
				else if (c=='D')
					lf();
				else if (c=='Z')
					respond(tty);
				else if (x=='7')
					save_cur();
				else if (x=='8')
					restore_cur();
				break;
			case 2:
				for(npar=0;npar<NPAR;npar++)
					par[npar]=0;
				npar=0;
				state=3;
				if ((ques=(c=='?')))
					break;
			case 3:
				if (c==';' && npar<NPAR-1) {
					npar++;
					break;
				} else if (c>='0' && c<='9') {
					par[npar]=10*par[npar]+c-'0';
					break;
				} else state=4;
			case 4:
				state=0;
				switch(c) {
					case 'G': case '`':
						if (par[0]) par[0]--;
						gotoxy(par[0],y);
						break;
					case 'A':
						if (!par[0]) par[0]++;
						gotoxy(x,y-par[0]);
						break;
					case 'B': case 'e':
						if (!par[0]) par[0]++;
						gotoxy(x,y+par[0]);
						break;
					case 'C': case 'a':
						if (!par[0]) par[0]++;
						gotoxy(x+par[0],y);
						break;
					case 'D':
						if (!par[0]) par[0]++;
						gotoxy(x-par[0],y);
						break;
					case 'E':
						if (!par[0]) par[0]++;
						gotoxy(0,y+par[0]);
						break;
					case 'F':
						if (!par[0]) par[0]++;
						gotoxy(0,y-par[0]);
						break;
					case 'd':
						if (par[0]) par[0]--;
						gotoxy(x,par[0]);
						break;
					case 'H': case 'f':
						if (par[0]) par[0]--;
						if (par[1]) par[1]--;
						gotoxy(par[1],par[0]);
						break;
					case 'J':
						csi_J(par[0]);
						break;
					case 'K':
						csi_K(par[0]);
						break;
					case 'L':
						csi_L(par[0]);
						break;
					case 'M':
						csi_M(par[0]);
						break;
					case 'P':
						csi_P(par[0]);
						break;
					case '@':
						csi_at(par[0]);
						break;
					case 'm':
						csi_m();
						break;
					case 'r':
						if (par[0]) par[0]--;
						if (!par[1]) par[1]=lines;
						if (par[0] < par[1] &&
						    par[1] <= lines) {
							top=par[0];
							bottom=par[1];
						}
						break;
					case 's':
						save_cur();
						break;
					case 'u':
						restore_cur();
						break;
				}
		}
	}
	set_cursor();
}

/*
 *  void con_init(void);
 *
 * This routine initalizes console interrupts, and does nothing
 * else. If you want the screen to clear, call tty_write with
 * the appropriate escape-sequece.
 */
void con_init(void)
{
	register unsigned char a;

	gotoxy(*(unsigned char *)(0x90000+510),*(unsigned char *)(0x90000+511));
	set_trap_gate(0x21,&keyboard_interrupt);
	outb_p(inb_p(0x21)&0xfd,0x21);
	a=inb_p(0x61);
	outb_p(a|0x80,0x61);
	outb(a,0x61);
}

void f1_down(void) {f1_down_flag = 1;}

void f1_up(void) {f1_down_flag = 0;}

void tool_draw(void)
{
	save_cur();
	
	char c = '#';
	int i, j;
	for(i = TOOL_COL_START; i < TOOL_COL_END - 1; i++) //UP COLUMN DRAW
	{
		gotoxy(i, 0);
		__asm__(
			"movb attr, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (c), "m" (*(short *)pos)
			);
	}
	for(i = TOOL_COL_START; i < TOOL_COL_END - 1; i++) //DOWN COLUMN DRAW
	{
		gotoxy(i, TOOL_LINE_END - 1);
		__asm__(
			"movb attr, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (c), "m" (*(short *)pos)
			);
	}
	for(i = TOOL_LINE_START; i < TOOL_LINE_END; i++) //LEFT LINE DRAW
	{
		gotoxy(TOOL_COL_START, i);
		__asm__(
			"movb attr, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (c), "m" (*(short *)pos)
			);
	}
	for(i = TOOL_LINE_START; i < TOOL_LINE_END; i++) //RIGH LINE DRAW
	{
		gotoxy(TOOL_COL_END - 1, i);
		__asm__(
			"movb attr, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (c), "m" (*(short *)pos)
			);
	}
	c = ' ';
	for(i = TOOL_LINE_START + 1; i < TOOL_LINE_END - 1; i++)
	{
		for(j = TOOL_COL_START + 1; j < TOOL_COL_END - 1; j++)
		{
		gotoxy(j, i);
		__asm__(
			"movb attr, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (c), "m" (*(short *)pos)
			);
		}
	}
	restore_cur();

}



static volatile struct m_inode *dir_inode;
static volatile struct m_inode *root_inode;

//File explorer
static volatile int type[10]; 
static volatile unsigned short inode[10];
static volatile char name[10][20];
static int selected_index = 0;
static volatile int list_count;

//Clipboard
volatile char character_for_input;
static volatile char text_clip[10][20];
static int selected_index_clip = 0;

static volatile int  list_count_clip;



static volatile char current_addres[20];

void tool_start(void)
{
	
	if(tool == 1)
		{
		tool = 2;
		tool_clip();
		return;
		}
	if(tool == 2)
		{
		tool = 1;
		//openR();
		tool_dir();
		//closeR();
		return;
		}
	if(tool == 0)
		{
		tool = 1;
		strcpy(current_addres, "/");
		openR();
		tool_dir();
		closeR();
		}
}

void openR(void)
{
		root_inode = iget(0x301, 1);
		current->root = root_inode;
		current->pwd = root_inode;
		dir_inode = namei(current_addres);	
}

void closeR(void)
{
		iput(root_inode);
		iput(dir_inode);
		current->root = NULL;
		current->pwd = NULL;
}


void tool_clip()
{
	tool_draw();
	set_path_name("clipboard");

	draw_list_clip();
	mark_selected_clip();
}

void tool_dir()
{
	tool_draw();
	set_path_name(current_addres);
	
	fill_list();
	
	draw_list();
	mark_selected();
}

void draw_list_clip()//BANE
{
	unsigned char col = 0x06;
	int i;
	save_cur();
	for(i = 0; i < 10; i++)
	{
		//if(strlen(text[i]) == 0)
		//	continue;
		int k = strlen(text_clip[i]);
		k /= 2;
		//printk(text[i]);
		//printk("\n");
		
		int col_write_start = TOOL_COL_START + (12 - k) - 1;
		int j;
		for(j = 0; j < strlen(text_clip[i]); j++)
		{
			char c = text_clip[i][j];
			gotoxy(col_write_start + j, i + 1);
			
			__asm__(
			"movb %2, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (c), "m" (*(short *)pos), "m" (col)
			);
		}
	}
	restore_cur();
}


void draw_list()
{
	if(list_count == 0)
		return;

	
	unsigned char col;

	int i;
	save_cur();
	for(i = 0; i < list_count; i++)
	{
		int j;
		if(type[i] == 0)
			col = regular;
		else if(type[i] == 1)
			col = dir;
		else if(type[i] == 2)
			col = exe_files;
		else if(type[i] == 3)
			col = dev;

		int k = strlen(name[i]);
		k /= 2;

		int col_write_start = TOOL_COL_START + (12 - k) - 1;
		for(j = 0; j < strlen(name[i]); j++)
		{
			char c = name[i][j];
			
			gotoxy(col_write_start + j, i + 1);

			__asm__(
			"movb %2, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (c), "m" (*(short *)pos), "g" (col)
			);

		}	
	}


	restore_cur();
	
}

void put_char_to_clip(void)
{
	if(strlen(text_clip[selected_index_clip]) < 19)
	{
	text_clip[selected_index_clip][strlen(text_clip[selected_index_clip])] = character_for_input;
	tool_draw(); // TEST
	draw_list_clip();
	mark_selected_clip();
	set_path_name("clipboard");
	}
}

void remove_char_for_clip(void)
{
	if(strlen(text_clip[selected_index_clip]) != 0)
	{
	text_clip[selected_index_clip][strlen(text_clip[selected_index_clip]) - 1] = '\0';
	tool_draw(); // TEST
	draw_list_clip();
	mark_selected_clip();
	set_path_name("clipboard");
	}
}

void paste_to_con(void)
{	
	int i;
	if(tool == 2)
	{
	
	for(i = 0; i < strlen(text_clip[selected_index_clip]); i++)
	{
		PUTCH(text_clip[selected_index_clip][i], tty_table[0].read_q);
	}
	}
	else
	{
	for(i = 0; i < strlen(current_addres); i++)
	{
		PUTCH(current_addres[i], tty_table[0].read_q);
	}
	for(i = 0; i < strlen(name[selected_index]); i++)
	{
		PUTCH(name[selected_index][i], tty_table[0].read_q);
	}
	}
}

void mark_selected_clip()
{
	int j;
	save_cur();
	for(j = TOOL_COL_START + 1; j < TOOL_COL_END - 1; j++)
	{
		unsigned char colour = 0x30;
		gotoxy(j, selected_index_clip + 1);
		__asm__(
			"movw %0, %%ax\n\t"
			"movb %1, %%ah\n\t"
			"movw %%ax, %0\n\t"
			:: "m" (*(short *)pos), "m" (colour) : "%ax"
			);

		
	}

	restore_cur();
}

void mark_selected()
{
	int j;


	save_cur();
	for(j = TOOL_COL_START + 1; j < TOOL_COL_END - 1; j++)
	{
		unsigned char colour = 0x30;
		gotoxy(j, selected_index + 1);
		__asm__(
			"movw %0, %%ax\n\t"
			"movb %1, %%ah\n\t"
			"movw %%ax, %0\n\t"
			:: "m" (*(short *)pos), "m" (colour) : "%ax"
			);

		
	}

	restore_cur();
}

void fill_list() //16877 dir, 33188 reg file, 33261 exe, 8685 dev
{
	struct dir_entry *entry;
	struct buffer_head *bh = bread(dir_inode->i_dev, dir_inode->i_zone[0]);
	
	entry = (struct dir_entry*) bh->b_data;


	int i = 0;
	while(i < 10)
	{
		if(entry->inode == 0)
			break;

		if(entry->name[0] != '.')
		{
		
		struct m_inode *node;
		node = iget(0x301, entry->inode);
		
		
		int type_f = -1;

		if(node->i_mode == 16877)
			type_f = 1;
		else if(node->i_mode == 33188)
			type_f = 0;
		else if(node->i_mode == 33261)
			type_f = 2;
		else if(node->i_mode == 8685)
			type_f = 3;

		type[i] = type_f;
		inode[i] = entry->inode;
		strcpy(name[i], entry->name);

		iput(node);
		i++;
		}
		entry++;
	}

	list_count = i;

}

void set_path_name(char const *pathname)
{
	int k = strlen(pathname);
	k /= 2;
	int col_write_start = TOOL_COL_START + (12 - k) - 1;
	save_cur();

	char c = '[';
	gotoxy(col_write_start-1, 0);
		__asm__(
			"movb attr, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (c), "m" (*(short *)pos)
			);

	int n = strlen(pathname);
	int i;
	for(i = 0; i < n; i++)
	{
		
	gotoxy(col_write_start + i, 0);
		__asm__(
			"movb attr, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (pathname[i]), "m" (*(short *)pos)
			);
	}

	c = ']';
	gotoxy(col_write_start + i, 0);
		__asm__(
			"movb attr, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (c), "m" (*(short *)pos)
			);
		

	
	restore_cur();
	
}

void arr_right(void)
{
	if(tool == 2)
		return;
	if(type[selected_index] != 1)
	{
		printk("Not a dir\n");
		return;
	}
	strcat(current_addres, name[selected_index]);
	strcat(current_addres, "/");
	openR();
	tool_draw();
	set_path_name(current_addres);
	fill_list();
	
	draw_list();
	if(list_count == 0)
		return;

	selected_index = 0;
	mark_selected();
	closeR();
	
}

void arr_left(void)
{
	if(tool == 2)
		return;
	if(strlen(current_addres) == 1)
		return;

	int i = strlen(current_addres) - 2;
	while(1)
	{
		if(current_addres[i] == '/')
		{
			current_addres[++i] = '\0';
			break;	
		}
		i--;
	}
	openR();
	tool_draw();
	set_path_name(current_addres);
	fill_list();
	
	draw_list();
	if(list_count == 0)
		return;

	selected_index = 0;
	mark_selected();
	closeR();
}


void arr_down(void)
{
	if(tool == 2)
	{
		if(selected_index_clip + 1 < 10)
		{
		selected_index_clip++;
		tool_draw();
		set_path_name("clipboard");
		draw_list_clip();
		mark_selected_clip();
		}
		else
		{
		selected_index_clip = 0;
		tool_draw();
		set_path_name("clipboard");
		draw_list_clip();
		mark_selected_clip();
		}
		return;
	}
	
	if((selected_index + 1) < list_count) {
		selected_index++;
		tool_draw();
		set_path_name(current_addres);
		draw_list();
		mark_selected();
	}
}

void arr_up(void)
{
	if(tool == 2)
	{
		if(selected_index_clip - 1 >= 0)
		{
		selected_index_clip--;
		tool_draw();
		set_path_name("clipboard");
		draw_list_clip();
		mark_selected_clip();
		}
		else
		{
		selected_index_clip = 9;
		tool_draw();
		set_path_name("clipboard");
		draw_list_clip();
		mark_selected_clip();
		}
		return;
	}

	if((selected_index - 1) >= 0) {
		selected_index--;
		tool_draw();
		set_path_name(current_addres);
		draw_list();
		mark_selected();
	}
}



void print_name_and_index(void)
{
	char name[30];
	strcpy(name, "Pavle Prica RN 75/2018");
	int i;
	save_cur();
	for(i = 0; i < 22; i++)
	{
		gotoxy(i, 0);
		__asm__(
			"movb special, %%ah\n\t"
			"movw %%ax, %1\n\t"
			:: "a" (name[i]), "m" (*(short *)pos)
			);
		
	}
	restore_cur();
}
