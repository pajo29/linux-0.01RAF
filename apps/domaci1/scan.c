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

static char mnemonic_key[SCAN_CODE_SIZE];
static char *mnemonic[SCAN_CODE_SIZE];


void check_for_files(int file_TBL, int file_MN);
void length_check(const char *buffer);


void load_config(const char *scancodes_filename, const char *mnemonic_filename)
{
    write_new_line();

    int file_TBL = open(scancodes_filename, O_RDONLY);
    int file_MN = open(mnemonic_filename, O_RDONLY);

    check_for_files(file_TBL, file_MN);

    char buffer_first_line[SCAN_CODE_SIZE];
    char buffer_second_line[SCAN_CODE_SIZE];

    int len = fgets(buffer_first_line, SCAN_CODE_SIZE, file_TBL);
        fgets(buffer_second_line, SCAN_CODE_SIZE, file_TBL);

    int i;
    for(i = 0; i < len; i++)
    {
        scan_code_firt_row[i] = buffer_first_line[i];
        scan_code_second_row[i] = buffer_second_line[i];
    }

    char mn_num[3];
    fgets(mn_num, 3, file_MN);

    len = atoi(mn_num);

    for(i = 0; i < len; i++)
    {
        int n = fgets(buffer_first_line, SCAN_CODE_SIZE, file_MN); //NE RADI, SREDITI
        mnemonic_key[i] = buffer_first_line[0];
        int j;
        char tmp[SCAN_CODE_SIZE];
        int k = 0;
        for(j = 2; j < n - 1; j++)
        {
            tmp[k++] = buffer_first_line[j];
        }
        tmp[k] = '\0';
        printstr("\n");
        printstr(tmp);
        strcpy(mnemonic[i], tmp);
    }

    close(file_TBL);
    close(file_MN);
}

int process_scancode(int scancode, char *buffer)
{
	int result;

	/*
		Your code goes here!
		Remember, only inline assembly.
		Good luck!
		P: Thanks! :D
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

/* void length_check(const char *buffer)
{
    if( > SCAN_CODE_SIZE)
    {
        printerr("Array in file to large. Exiting.");
        _exit(1);
    }
} */
