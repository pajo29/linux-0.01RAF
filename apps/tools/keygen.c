#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "../utils.h"

int main(char *args)
{

    int argc = get_argc(args);
    if(argc < 2)
    {
        write(1, "\nNedostaje argument za nivo generisanja kljuca. Prekid.\n", strlen("\nNedostaje argument za nivo generisanja kljuca. Prekid.\n"));
        _exit(1);
    }

    int level = atoi(get_argv(args, 1));

    if(level < 1 || level > 3)
    {
        write(1, "\nKljuc nije u odgovarajucem opsegu. Opseg je 1 - 3. Prekid.\n", strlen("\nKljuc nije u odgovarajucem opsegu. Opseg je 1 - 3. Prekid.\n"));
        _exit(1);
    }

    generate_key_(level);
    _exit(0);
}
