#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char *args)
{

    int argc = get_argc(args);
    if(argc < 2) _exit(1);

    if(argc > 2)
        write(1, "Previse argumenata, prihvata se samo prvi argument.\n", strlen("Previse argumenata, prihvata se samo prvi argument.\n"));

    int fd = open(get_argv(args, 1), O_RDWR);
    encr(fd);
    close(fd);
    _exit(0);
}
