#define __LIBRARY__
#include <unistd.h>

_syscall2(int,set_key,char *,key,int,len);

