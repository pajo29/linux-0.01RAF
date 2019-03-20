#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "scan.h"

#define UTIL_IMPLEMENTATION
#include "../utils.h"

#define BUFFER_SIZE 128
#define OUT_BUFFER_SIZE 1024

int end_of_file_check(const char *code);


int main(int argc, char *argv[])
{

    char buff_TBL[BUFFER_SIZE];
    char buff_MN[BUFFER_SIZE];

    printstr("Unesite naziv fajla sa tabelom scan kodova: ");

    //read(0, buff_TBL, BUFFER_SIZE);
    strcpy(buff_TBL, "scancodes1.tbl\n");

    write_new_line();

    printstr("Unesite naziv fajla sa mnemonicima: ");

    //read(0, buff_MN, BUFFER_SIZE);
    strcpy(buff_MN, "ctrl1.map\n");

    buff_TBL[strlen(buff_TBL) - 1] = '\0'; //Skracivanje '\n' sa kraja, jer read hvata i enter.
    buff_MN[strlen(buff_MN) - 1] = '\0';
    load_config(buff_TBL, buff_MN);

    //SCAN DATOTEKA
    char sc_file[BUFFER_SIZE];

    //printstr("\nUnesite naziv sc datoteke: ");
    //read(0, sc_file, BUFFER_SIZE);
    //sc_file[strlen(sc_file) - 1] = '\0';
    strcpy(sc_file, "testfajl1.tst");

    int file = open(sc_file, O_RDONLY);

    if(file == -1)
    {
        printstr("\nFile not found. Exiting.");
        _exit(1);
    }

    char code[4];
    char out_buffer[OUT_BUFFER_SIZE];

    while(end_of_file_check(code))
    {
        fgets(code, 4, file);
        process_scancode(atoi(code), out_buffer);
    }

    close(file);


    _exit(0);
}

int end_of_file_check(const char *code)
{
    return *code == '4' ? *++code == '0' ? *++code == '0' ? 0 : 1 : 1 : 1;
}
