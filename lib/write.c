#define __LIBRARY__
#include <unistd.h>


_syscall3(int,write,int,fd,const char *,buf,off_t,count)

_syscall1(int, switch_case, int, fd)


