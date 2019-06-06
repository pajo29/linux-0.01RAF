#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char *args)
{//sys.h (include,linux) unistd.h (include) write.c (lib)

    ispisi_test();

    _exit(0);
}
