#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "../utils.h"

int main(char *args)
{
    int argc = get_argc(args);
    if(argc > 1) _exit(1);

    write(1, "Set your password:\n", strlen("Set your password:\n"));
    turn_on_key_set();

    char buffer[1024];
    read(0, buffer, 1024);

    turn_off_key_set(); 

    buffer[strlen(buffer) - 1] = 0;
    set_key(buffer, strlen(buffer));

    _exit(0);
}
