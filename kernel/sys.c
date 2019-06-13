#include <errno.h>

#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <sys/times.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <string.h>


// static volatile int hashedGlobalKey = 0;
static volatile char global_key[100];
static volatile int is_key_set = 0;


static volatile int key_set_turn_on = 0;

short sys_get_i_node(int fd)
{

}

int sys_decr(int fd)
{
    if(is_key_set == 0)
    {
        printk("Key is not set.\n");
        return 0;
    }
    struct file * file;
    struct m_inode *inode;

    if (fd >= NR_OPEN || !(file=current->filp[fd]))
        return -EINVAL;

    inode = file->f_inode;

    if(check_for_encr(inode) == 0)
    {
        printk("File not encrypted.\n");
        return 1;
    }
    if(check_for_encr(inode) == -1)
    {
        printk("Nevalidan kljuc.\n");
        return 1;
    }

    
    file_decr(inode, file);
    return 0;
}

int remove_file_mark(struct m_inode *file_inode)
{
    struct m_inode *root = iget(0x301, 1);
    current->root = root;
    current->pwd = root;
    struct m_inode *dir = namei("/");

    struct dir_entry *entry;
    struct buffer_head *bhead = bread(root->i_dev, root->i_zone[0]);

    entry = (struct dir_entry*) bhead->b_data;

    struct m_inode *inode;
    int counter = 0, nr;
    struct buffer_head * bh;
    int flag = 0;

    while(1)
    {
        if(entry->inode == 0)
        {
            break;
        }

        if(compare_name(entry->name, ".fileList.txt"))
        {
            inode = iget(0x301, entry->inode);
            break;
        }
        entry++;
    }


    while (1) {
        if ((nr = bmap(inode, counter++))) {
            if (!(bh=bread(inode->i_dev,nr)))
                break;
        } else
            break;
        if (bh) {
            unmark(bh->b_data, 1024, file_inode->i_num);
            bh->b_dirt = 1;
            brelse(bh);
            break;
        }
    }
    inode->i_atime = CURRENT_TIME;

    //iput(inode);
    current->root = NULL;
    current->pwd = NULL;
    return 0;
}

int unmark(char *buffer, int len, int inode_num)
{
    int counter = 0;
    int tmp = reverse_num(inode_num);
    int num_count = 0;

    int found;
    char c;
    while(1)
    {
        num_count = 0;
        found = 1;
        while(tmp != 0)
        {
            c = (tmp % 10) + '0';
            tmp = tmp / 10;
            num_count++;
            if(c != buffer[counter++])
                found = 0;
        }
        counter++;
        while(buffer[counter] != ' ')
        {
            counter++;
            num_count++;
        }
        counter++;
        num_count++;

    
        tmp = reverse_num(inode_num);
        if(found == 1 || counter >= len)
            break;
    }

    if(found == 1)
    {
        int found_mark = counter - num_count;
        found_mark--;

        int k;
        for(k = found_mark; k < (found_mark + num_count); k++) 
            buffer[k] = ' ';

        for(k = found_mark; k < (len - num_count); k++)
                buffer[k] = buffer[k + num_count + 1];

        k--;
        while(k < len)
        {
            buffer[k++] = ' ';
        }
    }

}

int unmarkOld(char *buffer, int len, int inode_num) //TODO FINISH UMARK, MAYBE RETYPE IT
{
    int counter = 0;
    int tmp = reverse_num(inode_num);
    int num_count = 0;


    int found;
    char c;
    while(1)
    {
        found = 1;
        while(tmp != 0)
        {
            c = (tmp % 10) + '0';
            tmp = tmp / 10;
            num_count++;
            if(c != buffer[counter++])
                found = 0;
        }
        counter++;
        if(found = 1 || counter >= len)
            break;
    }

    if(found == 1)
    {
        if(buffer[counter + 1 + num_count] == ' ')
        {
            int found_mark = --counter - num_count;
            int i;
            for(i = 0; i <= num_count; i++)
            {
                buffer[counter++] = ' ';
            }
        }
        else
        {
            char old_buffer[len];
            copy_to_buffer(old_buffer, buffer, len);
            int found_mark = --counter - num_count;

            int i, j = 0;
            for(i = 0; i < len; i++)
            {
                if(i < found_mark || i > counter)
                    buffer[i] = old_buffer[j++];
                else
                {
                    j = j + num_count + 1;
                    while(i <= counter)
                    {
                        buffer[i++] = old_buffer[j++];
                    }
                    i--;
                }
            }
        }
    }

    return 0;
}

int file_decr(struct m_inode *inode)
{
    int chars,nr;
	struct buffer_head * bh;

    int counter = 0;

    while (1) {
         if ((nr = bmap(inode, counter++))) {
            if (!(bh=bread(inode->i_dev,nr)))
                break;
        } else
            break;
        if (bh) {
            buffer_decr(bh->b_data, 1024);
            if(S_ISDIR(inode->i_mode)) {
            struct dir_entry *entry = (struct dir_entry*) bh->b_data;
            int entries = inode->i_size / (sizeof(struct dir_entry));
            entries -= 2;
            // printk("%d : entries  ", entries);
            entry++;
            entry++;
            while(entries > 0) {
                struct m_inode *node;

                if(entry->name[0] != '.') {
                node = iget(0x301, entry->inode);
                file_decr(node);
                iput(node);
                entries--;
            }
                entry++;
            }

            }
            bh->b_dirt = 1;
            brelse(bh);
        }
    }


	inode->i_atime = CURRENT_TIME;
    remove_file_mark(inode);
	return 0;
}

int buffer_decr(char *buffer, int len)
{
    char old_buffer[len];
    copy_to_buffer(old_buffer, buffer, len);
    int gb = strlen(global_key);

    int index_array[gb];

    int i;
    for(i = 0; i < gb; i++)
        index_array[i] = i;

    char global_key_local[gb];
    to_lower_case(global_key, global_key_local);

    sort_index_and_global(index_array, global_key_local, 0, gb - 1);

    int counter = 0;
    for(i = 0; i < gb; i++)
    {
        int num = index_array[i];
        while(num < len)
        {
            buffer[num] = old_buffer[counter++];
            num += gb;
        }
    }

    return 0;
}

int sys_encr(int fd)
{
    if(is_key_set == 0)
    {
        printk("Key is not set.\n");
        return 0;
    }
    struct file * file;
    struct m_inode *inode;

    if (fd >= NR_OPEN || !(file=current->filp[fd]))
        return -EINVAL;

    inode = file->f_inode;

    if(check_for_encr(inode) == 1)
    {
        printk("File already encrypted.\n");
        return 0;
    }

    if(check_for_encr(inode) == -1)
    {
        printk("Wrong key.\n");
        return 0;
    }
    

    file_encr(inode);
    return 0;
}

int is_key_set_()
{
    return is_key_set;
}

int mark_file(struct m_inode *file_inode)
{
    struct m_inode *root = iget(0x301, 1);
    current->root = root;
    current->pwd = root;
    struct m_inode *dir = namei("/");

    struct dir_entry *entry;
	struct buffer_head *bhead = bread(root->i_dev, root->i_zone[0]);

	entry = (struct dir_entry*) bhead->b_data;

    struct m_inode *inode;
    int counter = 0, nr;
    struct buffer_head * bh;
    int flag = 0;

    while(1)
    {
        if(entry->inode == 0)
        {
            break;
        }

        if(compare_name(entry->name, ".fileList.txt"))
        {
            inode = iget(0x301, entry->inode);
            break;
        }
        entry++;
    }


    while (1) {
        if ((nr = bmap(inode, counter++))) {
            if (!(bh=bread(inode->i_dev,nr)))
                break;
        } else
            break;
        if (bh) {
            mark(bh->b_data, 1024, file_inode->i_num);
            bh->b_dirt = 1;
            brelse(bh);
            break;
        }
    }
    inode->i_atime = CURRENT_TIME;

    //iput(inode);
    current->root = NULL;
    current->pwd = NULL;
    return 0;
}

int mark(char *buffer, int len, int inode_num)
{
    if(buffer[0] < '0' || buffer[0] > '9')
    {
        inode_num = reverse_num(inode_num);
        int counter = 0;
        while(inode_num != 0)
        {
            buffer[counter++] = (inode_num % 10) + '0';
            inode_num = inode_num / 10;
        }
        buffer[counter++] = '~';

        int hashedGlobalKey = reverse_num(hash(global_key));
        while(hashedGlobalKey != 0)
        {
            buffer[counter++] = (hashedGlobalKey % 10) + '0';
            hashedGlobalKey /= 10;
        }

        buffer[counter] = ' ';

        return;
    }
    else
    {
    int counter = 0;
    int set = 0;
    while(set != 1)
    {
        set = 1;
        while(buffer[counter] != ' ')
            counter++;
        if(buffer[counter + 1] != ' ')
        {
            set = 0;
            counter++;
        }
    }

    counter++;
    inode_num = reverse_num(inode_num);
        while(inode_num != 0)
        {
            buffer[counter++] = (inode_num % 10) + '0';
            inode_num = inode_num / 10;
        }
        buffer[counter++] = '~';

        int hashedGlobalKey = reverse_num(hash(global_key));
        while(hashedGlobalKey != 0)
        {
            buffer[counter++] = (hashedGlobalKey % 10) + '0';
            hashedGlobalKey /= 10;
        }

        buffer[counter] = ' ';
    }
}

int reverse_num(int num)
{
    int reverse_num = 0;
    while(num != 0)
    {
        reverse_num = reverse_num * 10 + (num % 10);
        num = num / 10;
    }
    return reverse_num;
}

int check_for_encr(struct m_inode * file_inode)
{
    struct m_inode *root = iget(0x301, 1);
    current->root = root;
    current->pwd = root;
    struct m_inode *dir = namei("/");

    struct dir_entry *entry;
	struct buffer_head *bhead = bread(root->i_dev, root->i_zone[0]);

	entry = (struct dir_entry*) bhead->b_data;

    struct m_inode *inode;
    int counter = 0, nr;
    struct buffer_head * bh;
    int flag = 0;

    while(1)
    {
        if(entry->inode == 0)
        {
            break;
        }

        if(compare_name(entry->name, ".fileList.txt"))
        {
            inode = iget(0x301, entry->inode);
            break;
        }
        entry++;
    }


    while (1) {
        if ((nr = bmap(inode, counter++))) {
            if (!(bh=bread(inode->i_dev,nr)))
                break;
        } else
            break;
        if (bh) {
            flag = i_node_check(bh->b_data, 1024, file_inode);
            bh->b_dirt = 1;
            brelse(bh);
            if(flag == 1 || flag == -1 || flag == 0)
                break;
        }
    }
    inode->i_atime = CURRENT_TIME;

    //iput(inode);
    current->root = NULL;
    current->pwd = NULL;
    return flag;
}

int compare_name(char *name, char *name_two)
{
    
    int i;
    for(i = 0; i < strlen(name_two); i++) {
        if(name[i] != name_two[i])
            return 0;
    }
    return 1;
}

int i_node_check(char *buffer, int len, struct m_inode *file_inode)
{
    unsigned short num = 0;

    int i;
    for(i = 0; i < len; i++)
    {
        if(buffer[i] == '~')
        {
            if(num == file_inode->i_num) 
            {
                int pass = 0;
                int k = 0;
                i++;
                while(buffer[i] != ' ') 
                {
                    pass = (pass * 10) + (buffer[i++] - '0');
                }

                printk("%d : %d\n", pass, hash(global_key));
                printk(global_key);
                printk("\n");

                if(hash(global_key) != pass) {
                    printk("JEL SE OVO DESILO");
                    return -1;
                }
                return 1;
            }
            else
            {
                while(buffer[i] != ' ')
                    i++;
            }
            num = 0;
        }
        else
        {
            num = num * 10 + (buffer[i] - '0');
        }
    }
    return 0;
}

int file_encr(struct m_inode * inode)
{
    printk(global_key);
    printk(": GLOBALNI KLJUC\n"); //KOJI KURAC??
    int left,chars,nr;
	struct buffer_head * bh;

    int counter = 0;

	while (1) {
         if ((nr = bmap(inode, counter++))) {
            if (!(bh=bread(inode->i_dev,nr)))
                break;
        } else
            break;
        if (bh) {
            if(S_ISDIR(inode->i_mode)) {
            printk("USAO U DIR");
            struct dir_entry *entry = (struct dir_entry*) bh->b_data;
            int entries = inode->i_size / (sizeof(struct dir_entry));
            entries -= 2;
            // printk("%d : entries  ", entries);
            entry++;
            entry++;
            while(entries > 0) {
                struct m_inode *node;

                if(entry->name[0] != '.') {
                node = iget(0x301, entry->inode);
                file_encr(node);
                iput(node);
                entries--;
            }
                entry++;
            }

            }
            buffer_encr(bh->b_data, 1024);
            bh->b_dirt = 1;
            brelse(bh);
            // break;
        }
	}
    inode->i_atime = CURRENT_TIME;
    mark_file(inode);
	return 0;
}

int buffer_encr(char *buffer, int len)
{
    char old_buffer[len];
    copy_to_buffer(old_buffer, buffer, len);
    int gb = strlen(global_key);

    int index_array[gb];

    int i;
    for(i = 0; i < gb; i++)
        index_array[i] = i;

    char global_key_local[gb];
    to_lower_case(global_key, global_key_local);

    sort_index_and_global(index_array, global_key_local, 0, gb - 1);

    int counter = 0;
    for(i = 0; i < gb; i++)
    {
        int num = index_array[i];
        while(num < len)
        {
            buffer[counter++] = old_buffer[num];
            num += gb;
        }
    }

    return 0;
}

void copy_to_buffer(char *old_buffer, char *buffer, int len)
{
    int i = 0;
    for(i = 0; i < len; i++)
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

    printk("Generisan kljuc: ");

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
            char c = get_number;
            printk("%c", c);
            count--;
        }

    }
    printk("\n");
    return 0;
}

int sys_set_key(char *key, int len)
{
    key_set_turn_on = 0;
    if(len > 1024 || pow_check(len) == 0)
    {
        printk("Nepravilan unos.\n");
        return 0;
    }
    clear_key_();
    is_key_set = 1;
    static int flag = 1;//?

    char c, kerKey[100];
    int i;
    for(i = 0; i < len; i++) {
        c = get_fs_byte(key + i);
        kerKey[i] = c;
    }

    activate_timer();
    return 0;
}

int hash(char *str) 
{
    int hash = 5381;
    int c;

    while(c = *str++)
        hash = ((hash << 5) + hash) + c;

    return hash;
}



int sys_turn_on_key_set(void) {

    key_set_turn_on = 1;
    return 0;
}

int sys_turn_off_key_set(void) {

    key_set_turn_on = 0;
    return 0;
}

int sys_get_turn_on() {
    return key_set_turn_on;
}

int pow_check(int len)
{
    int i;
    int num = 1;
    for(i = 0; i < 10; i++)
    {
        if(num == len)
            return 1;
        num *= 2;
    }
    return 0;
}

int sys_clear_key(void)
{
    if(is_key_set == 0) {
        return 0;
    }
    clear_key_();
    is_key_set = 0;
    // printk("Kljuc izbrisan..\n");
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

