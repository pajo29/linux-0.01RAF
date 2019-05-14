#define __LIBRARY__
#include <unistd.h>

_syscall2(int,set_key,char *,key,int,len);

_syscall0(int,clear_key);

