#include <errno.h>
#include <fcntl.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


int buffer_switch_case(char *s, int l)
{
    int i;
    for(i = 0; i < l; i++)
    { //CHANGE CASE
        char ch = *s;
        if(ch >= 'a' && ch <= 'z')
            ch -= 'a' - 'A';
        else if(ch >= 'A' && ch <= 'Z')
            ch -= 'A' - 'a';

            *(s++) = ch;
    }
    return 0;
}

int sys_switch_case(int fd)
{
    struct file * file;
    struct m_inode *inode;

    if (fd >= NR_OPEN || !(file=current->filp[fd]))
        return -EINVAL;

    inode = file->f_inode;
    return file_switch_case(inode, file);
}

int file_switch_case(struct m_inode * inode, struct file * filp, char * buf, int count)
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
			buffer_switch_case(bh->b_data, 1024);
			bh->b_dirt = 1;
			brelse(bh);
		}
	}
	inode->i_atime = CURRENT_TIME;
	return 0;
}




int file_read(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	int left,chars,nr;
	struct buffer_head * bh;

	int test = 0;

	if(inode->i_num == 133)
	{
	    printk("Pristup odbijen.\n");
	    return 0;
	}

	// if(check_for_encr(inode) == 1)
	// {
		
	// }


	if ((left=count)<=0)
		return 0;
	while (left) {
		if ((nr = bmap(inode,(filp->f_pos)/BLOCK_SIZE))) {
			if (!(bh=bread(inode->i_dev,nr)))
				break;
		} else
			bh = NULL;
		nr = filp->f_pos % BLOCK_SIZE;
		chars = MIN( BLOCK_SIZE-nr , left );
		filp->f_pos += chars;
		left -= chars;
		if (bh) {
			char * p = nr + bh->b_data;
			while (chars-->0)
				put_fs_byte(*(p++),buf++);
			brelse(bh);
		} else {
			while (chars-->0)
				put_fs_byte(0,buf++);
		}
	}
	inode->i_atime = CURRENT_TIME;
	return (count-left)?(count-left):-ERROR;
}

int file_write(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	off_t pos;
	int block,c;
	struct buffer_head * bh;
	char * p;
	int i=0;

	if(inode->i_num == 133)
	{
	    printk("Pristup odbijen.\n");
	    return 0;
	}



/*
 * ok, append may not work when many processes are writing at the same time
 * but so what. That way leads to madness anyway.
 */
	if (filp->f_flags & O_APPEND)
		pos = inode->i_size;
	else
		pos = filp->f_pos;
	while (i<count) {
		if (!(block = create_block(inode,pos/BLOCK_SIZE)))
			break;
		if (!(bh=bread(inode->i_dev,block)))
			break;
		c = pos % BLOCK_SIZE;
		p = c + bh->b_data;
		bh->b_dirt = 1;
		c = BLOCK_SIZE-c;
		if (c > count-i) c = count-i;
		pos += c;
		if (pos > inode->i_size) {
			inode->i_size = pos;
			inode->i_dirt = 1;
		}
		i += c;
		while (c-->0) //Ovde se vrsi upis i moguca izmena. Na primer za vezbu 1 ovde se kuca.
			*(p++) = get_fs_byte(buf++);
		brelse(bh);
	}
	inode->i_mtime = CURRENT_TIME;
	if (!(filp->f_flags & O_APPEND)) {
		filp->f_pos = pos;
		inode->i_ctime = CURRENT_TIME;
	}
	return (i?i:-1);
}

