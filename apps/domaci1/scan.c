#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "../utils.h"
/*
	Deklaracije za nizove i tabele ovde
	tabelu za prevodjenje scancodeova i tabelu
	za cuvanje mnemonika cuvati kao staticke
	varijable unutar ovog fajla i ucitavati
	ih uz pomoc funkcije load_config

*/

#define SCAN_CODE_SIZE 128

static char scan_code_firt_row[SCAN_CODE_SIZE];
static char scan_code_second_row[SCAN_CODE_SIZE];

void check_for_files(int file_TBL, int file_MN);

void load_config(const char *scancodes_filename, const char *mnemonic_filename)
{
    write_new_line();

    int file_TBL = open(scancodes_filename, O_RDONLY);
    int file_MN = open(mnemonic_filename, O_RDONLY);

    check_for_files(file_TBL, file_MN);



}

int process_scancode(int scancode, char *buffer)
{
	int result;

	/*
		Your code goes here!
		Remember, only inline assembly.
		Good luck!
	*/

	return result;
}

void check_for_files(int file_TBL, int file_MN)
{
    if(file_TBL == -1)
    {
        printerr("\nScan code file not found. Exiting.");
        _exit(1);
    }
    else
        printstr("\nScan code file found");

    if(file_MN == -1)
    {
        printerr("\nMnemonic file not found. Exiting.");
        _exit(1);
    }
    else
        printstr("\nMnemonic file found.");

    write_new_line();
}
