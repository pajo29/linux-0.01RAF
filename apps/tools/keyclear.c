#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "../utils.h"

int main(char *args)
{

    int argc = get_argc(args);
    if(argc >= 2) write(1, "\nArgument nije prihvacen. Brisanje kljuca..\n", strlen("\nArgument nije prihvacen. Brisanje kljuca..\n"));

    clear_key();
    _exit(0);
}
