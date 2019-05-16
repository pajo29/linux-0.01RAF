#include <errno.h>

#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <sys/times.h>
#include <sys/utsname.h>
#include <string.h>

static volatile char global_key[100];


int sys_encr(int fd)
{
    struct file * file;
    struct m_inode *inode;

    if (fd >= NR_OPEN || !(file=current->filp[fd]))
        return -EINVAL;

    inode = file->f_inode;

    file_encr(inode, file);
    return 0;
}

int file_encr(struct m_inode * inode, struct file * filp, char * buf, int count)
{
    int left,chars,nr;
	struct buffer_head * bh;

	if ((left=count)<=0)
		return 0;
	while (left) {
		if ((nr = bmap(inode,(filp->f_pos)/BLOCK_SIZE))) {
			if (!(bh=bread(inode->i_dev,nr)))
				break;
		} else
			break;
		nr = filp->f_pos % BLOCK_SIZE;
		chars = BLOCK_SIZE-nr;
		filp->f_pos += chars;
		left -= chars;
		if (bh) {
			buffer_encr(bh->b_data, 1024);
			bh->b_dirt = 1;
			brelse(bh);
		}
	}
	inode->i_atime = CURRENT_TIME;
	return 0;
}

int buffer_encr(char *buffer, int len)
{

    int n = strlen(buffer);
    char old_buffer[n];
    copy_to_buffer(old_buffer, buffer);
    int gb = strlen(global_key);

    int index_array[gb];

    int i;
    for(i = 0; i < gb; i++)
        index_array[i] = i;

    char global_key_local[gb];
    to_lower_case(global_key, global_key_local);

    sort_index_and_global(index_array, global_key_local, 0, gb);

    int counter = 0;
    for(i = 0; i < gb; i++)
    {
        int num = index_array[i];
        while(num < strlen(old_buffer))
        {
            buffer[counter++] = old_buffer[num];
            num += gb;
        }
    }

    return 0;
}

void copy_to_buffer(char *old_buffer, char *buffer)
{
    int i = 0;
    for(i = 0; i < strlen(buffer); i++)
        old_buffer[i] = buffer[i];
}

void sort_index_and_global(int *index_array, char *global_key_local, int low, int high)
{
    if(low < high)
    {
        int part = partition(index_array, global_key_local, low, high);

        sort_index_and_global(index_array, global_key_local, low, part - 1);
        sort_index_and_global(index_array, global_key_local, part + 1, high);
    }
}

int partition(int *index_array, char *global_key_local, int low, int high)
{
    char pivot = global_key_local[high];
    int i = (low - 1);

    int j;
    for(j = low; j < high; j++)
    {
        if(global_key_local[j] <= pivot)
        {
            i++;

            char tmp = global_key_local[i];
            global_key_local[i] = global_key_local[j];
            global_key_local[j] = tmp;

            int numTmp = index_array[i];
            index_array[i] = index_array[j];
            index_array[j] = numTmp;
        }
    }

    char tmp = global_key_local[i + 1];
    global_key_local[i + 1] = global_key_local[high];
    global_key_local[high] = tmp;

    int numTmp = index_array[i + 1];
    index_array[i + 1] = index_array[high];
    index_array[high] = numTmp;

    return i + 1;
}

void to_lower_case(char* buffer, char* new_string)
{
    int i;
    for(i = 0; i < strlen(buffer); i++)
    {
        if(buffer[i] >= 65 && buffer[i] <= 90)
            new_string[i] = buffer[i] + 32;
        else
            new_string[i] = buffer[i];
    }
}

int sys_generate_key_(int level)
{
    int count = 4;
    while(level > 1)
    {
        count = count * 2;
        level--;
    }
    int i = 0;

    int number = CURRENT_TIME * (-1);
    int add_number = 4375;
    while(count > 0)
    {
        if(number == 0)
        {
            number = CURRENT_TIME * (-1) + add_number;
            add_number += 4375;
        }

        int get_number = number % 100;
        number = number / 100;

        if((get_number >= 65 && get_number <= 90) || (get_number >= 97 && get_number <= 122))
        {
            global_key[i++] = get_number;
            count--;
        }

    }

    printk("Generisan kljuc: ");
    int k;
    for(k = 0; k < i; k++)
    {
        printk("%c", global_key[k]);
    }
    printk("\n");
    return 0;
}

int sys_set_key(char *key, int len)
{
    clear_key_();

    char c;
    int i;
    for(i = 0; i < len; i++) {
        c = get_fs_byte(key + i);
        global_key[i] = c;
    }

    return 0;
}

int sys_clear_key(void)
{
    clear_key_();
    printk("Kljuc izbrisan..\n");
    return 0;
}


int clear_key_()
{
    int i;
    for(i = 0; i < 100; i++)
    {
        global_key[i] = 0;
    }
    return 0;
}

int sys_ftime()
{
	return -ENOSYS;
}

int sys_mknod()
{
	return -ENOSYS;
}

int sys_break()
{
	return -ENOSYS;
}

int sys_mount()
{
	return -ENOSYS;
}

int sys_umount()
{
	return -ENOSYS;
}

int sys_ustat(int dev,struct ustat * ubuf)
{
	return -1;
}

int sys_ptrace()
{
	return -ENOSYS;
}

int sys_stty()
{
	return -ENOSYS;
}

int sys_gtty()
{
	return -ENOSYS;
}

int sys_rename()
{
	return -ENOSYS;
}

int sys_prof()
{
	return -ENOSYS;
}

int sys_setgid(int gid)
{
	if (current->euid && current->uid)
		if (current->gid==gid || current->sgid==gid)
			current->egid=gid;
		else
			return -EPERM;
	else
		current->gid=current->egid=gid;
	return 0;
}

int sys_acct()
{
	return -ENOSYS;
}

int sys_phys()
{
	return -ENOSYS;
}

int sys_lock()
{
	return -ENOSYS;
}

int sys_mpx()
{
	return -ENOSYS;
}

int sys_ulimit()
{
	return -ENOSYS;
}

int sys_time(long * tloc)
{
	int i;

	i = CURRENT_TIME;
	if (tloc) {
		verify_area(tloc,4);
		put_fs_long(i,(unsigned long *)tloc);
	}
	return i;
}

int sys_setuid(int uid)
{
	if (current->euid && current->uid)
		if (uid==current->uid || current->suid==current->uid)
			current->euid=uid;
		else
			return -EPERM;
	else
		current->euid=current->uid=uid;
	return 0;
}

int sys_stime(long * tptr)
{
	if (current->euid && current->uid)
		return -1;
	startup_time = get_fs_long((unsigned long *)tptr) - jiffies/HZ;
	return 0;
}

int sys_times(struct tms * tbuf)
{
	if (!tbuf)
		return jiffies;
	verify_area(tbuf,sizeof *tbuf);
	put_fs_long(current->utime,(unsigned long *)&tbuf->tms_utime);
	put_fs_long(current->stime,(unsigned long *)&tbuf->tms_stime);
	put_fs_long(current->cutime,(unsigned long *)&tbuf->tms_cutime);
	put_fs_long(current->cstime,(unsigned long *)&tbuf->tms_cstime);
	return jiffies;
}

int sys_brk(unsigned long end_data_seg)
{
	if (end_data_seg >= current->end_code &&
	    end_data_seg < current->start_stack - 16384)
		current->brk = end_data_seg;
	return current->brk;
}

/*
 * This needs some heave checking ...
 * I just haven't get the stomach for it. I also don't fully
 * understand sessions/pgrp etc. Let somebody who does explain it.
 */
int sys_setpgid(int pid, int pgid)
{
	int i;

	if (!pid)
		pid = current->pid;
	if (!pgid)
		pgid = pid;
	for (i=0 ; i<NR_TASKS ; i++)
		if (task[i] && task[i]->pid==pid) {
			if (task[i]->leader)
				return -EPERM;
			if (task[i]->session != current->session)
				return -EPERM;
			task[i]->pgrp = pgid;
			return 0;
		}
	return -ESRCH;
}

int sys_getpgrp(void)
{
	return current->pgrp;
}

int sys_setsid(void)
{
	if (current->uid && current->euid)
		return -EPERM;
	if (current->leader)
		return -EPERM;
	current->leader = 1;
	current->session = current->pgrp = current->pid;
	current->tty = -1;
	return current->pgrp;
}

int sys_oldolduname(void* v)
{
	printk("calling obsolete system call oldolduname\n");
	return -ENOSYS;
//	return (0);
}

int sys_uname(struct utsname * name)
{
	static struct utsname thisname = {
		"linux 0.01-3.x","nodename","release ","3.x","i386"
	};
	int i;

	if (!name) return -1;
	verify_area(name,sizeof *name);
	for(i=0;i<sizeof *name;i++)
		put_fs_byte(((char *) &thisname)[i],i+(char *) name);
	return (0);
}

int sys_umask(int mask)
{
	int old = current->umask;

	current->umask = mask & 0777;
	return (old);
}

int sys_null(int nr)
{
	static int prev_nr=-2;
	if (nr==174 || nr==175) return -ENOSYS;

	if (prev_nr!=nr)
	{
		prev_nr=nr;
//		printk("system call num %d not available\n",nr);
	}
	return -ENOSYS;
}

