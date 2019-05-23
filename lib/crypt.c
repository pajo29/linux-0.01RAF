#define __LIBRARY__
#include <unistd.h>

_syscall2(int,set_key,char *,key,int,len);

_syscall0(int,clear_key);

_syscall1(int,generate_key_,int,level);

_syscall1(int,encr,int,fd);

_syscall1(int,decr,int,fd);

_syscall1(short,get_i_node,int,fd);
