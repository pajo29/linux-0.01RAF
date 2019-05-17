#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "../utils.h"

int main(char *args)
{
    int argc = get_argc(args);
    if(argc < 2) _exit(1);


    char *key = get_argv(args, 1);

    set_key(key, strlen(key));

    _exit(0);
}
