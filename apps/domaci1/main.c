#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "scan.h"

#define UTIL_IMPLEMENTATION
#include "../utils.h"

#define BUFFER_SIZE 128
#define OUT_BUFFER_SIZE 1024


int main(int argc, char *argv[])
{

    char buff_TBL[BUFFER_SIZE];
    char buff_MN[BUFFER_SIZE];

    printstr("Unesite naziv fajla sa tabelom scan kodova: ");
    write_new_line();

    read(0, buff_TBL, BUFFER_SIZE);

    write_new_line();

    printstr("Unesite naziv fajla sa mnemonicima: ");
    write_new_line();

    read(0, buff_MN, BUFFER_SIZE);

    buff_TBL[strlen(buff_TBL) - 1] = '\0'; //Skracivanje '\n' sa kraja, jer read hvata i enter.
    buff_MN[strlen(buff_MN) - 1] = '\0';
    load_config(buff_TBL, buff_MN);

    char sc_file[BUFFER_SIZE];

    printstr("\nUnesite naziv sc datoteke: ");
    read(0, sc_file, BUFFER_SIZE);
    sc_file[strlen(sc_file) - 1] = '\0';

    int file = open(sc_file, O_RDONLY);

    if(file == -1)
    {
        printstr("Fajl ne postoji.");
        _exit(1);
    }

    char code[3];
    char out_buffer[OUT_BUFFER_SIZE];

    char test1[3];
    strcpy(test1, "400");
    char test[100];
    fgets(code, 3, file);
    code[strlen(code) - 1] = '\0';
    itoa(strcmp(test1, code), test);
    printstr(test);

    /*while(strcmp(code, "400") != 0)
    {
        fgets(code, 3, file);
        process_scancode(atoi(code), out_buffer);
        write_new_line();
        printstr(code);
    }*/

    close(file);





    _exit(0);


}
