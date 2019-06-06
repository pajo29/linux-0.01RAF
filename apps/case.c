#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char *args)
{//sys.h (include,linux) unistd.h (include) write.c (lib)

    int argc = get_argc(args);
    if(argc < 2) _exit(1);

    int fd = open(get_argv(args, 1), O_RDWR);
    switch_case(fd);
    close(fd);
    _exit(0);
}
