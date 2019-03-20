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

static volatile int shift_flag;
static volatile int ctrl_flag;
static volatile int alt_flag;


void check_for_files(int file_TBL, int file_MN);


void load_config(const char *scancodes_filename, const char *mnemonic_filename)
{
    shift_flag = ctrl_flag = alt_flag = 0;
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
        int n = fgets(buffer_first_line, SCAN_CODE_SIZE, file_MN);
        mnemonic_key[i] = buffer_first_line[0];
        int j;
        char tmp[SCAN_CODE_SIZE];
        int k = 0;
        for(j = 2; j < n - 1; j++)
        {
            tmp[k++] = buffer_first_line[j];
        }
        tmp[k] = '\0';
        strcpy(mnemonic[i], tmp);
    }

    close(file_TBL);
    close(file_MN);
}

int process_scancode(int scancode, char *buffer)
{
	int result = 0;
	int a;

    __asm__ __volatile__(
            /*"cld;"
            "movl %15, %%eax;"
            "movl %6, %%ebx;"
            "cmpl %%eax, %%ebx;"
            "je end_of_file;"*/ //TO BE USED WHEN FILE REACHES 400

            ".section .text;"
            "movl %14, %%edx;" //flag_used

            //CTRL/SHIFT/ALT TESTS

            "cld;" //shift down compare
            "pushl %%eax;"
            "pushl %%edx;"
            "movl %6, %%eax;"
            "movl %7, %%edx;"
            "cmpl %%eax, %%edx;"
            "popl %%edx;"
            "popl %%eax;"
            "je shift_down;"

            "cld;" //shift up compare
            "pushl %%eax;"
            "pushl %%edx;"
            "movl %6, %%eax;"
            "movl %8, %%edx;"
            "cmpl %%eax, %%edx;"
            "popl %%edx;"
            "popl %%eax;"
            "je shift_up;"

            "cld;" //ctrl down compare
            "pushl %%eax;"
            "pushl %%edx;"
            "movl %6, %%eax;"
            "movl %9, %%edx;"
            "cmpl %%eax, %%edx;"
            "popl %%edx;"
            "popl %%eax;"
            "je ctrl_down;"

            "cld;" //ctrl up compare
            "pushl %%eax;"
            "pushl %%edx;"
            "movl %6, %%eax;"
            "movl %10, %%edx;"
            "cmpl %%eax, %%edx;"
            "popl %%edx;"
            "popl %%eax;"
            "je ctrl_up;"

            "cld;" //alt down compare
            "pushl %%eax;"
            "pushl %%edx;"
            "movl %6, %%eax;"
            "movl %11, %%edx;"
            "cmpl %%eax, %%edx;"
            "popl %%edx;"
            "popl %%eax;"
            "je alt_down;"

            "cld;" //alt up compare
            "pushl %%eax;"
            "pushl %%edx;"
            "movl %6, %%eax;"
            "movl %12, %%edx;"
            "cmpl %%eax, %%edx;"
            "popl %%edx;"
            "popl %%eax;"
            "je alt_up;"

            "jmp no_flags;"

            //FLAG_JUMPS

            "shift_down:"
            "movl %13, %%eax;"
            "movl %13, %%edx;"
            "jmp end;"

            "shift_up:"
            "movl %14, %%eax;"
            "movl %13, %%edx;"
            "jmp end;"

            "ctrl_down:"
            "movl %13, %%ebx;"
            "movl %13, %%edx;"
            "jmp end;"

            "ctrl_up:"
            "movl %14, %%ebx;"
            "movl %13, %%edx;"
            "jmp end;"

            "alt_down:"
            "movl %13, %%ecx;"
            "movl %13, %%edx;"
            "jmp end;"

            "alt_up:"
            "movl %14, %%ecx;"
            "movl %13, %%edx;"
            "jmp end;"

            "no_flags:"
            "pushl %%edx;" //POPOVATI OBAVEZNO
            //"movl $0x0, %%edx;" //Using dx as counter for result

            "cld;"
            "movl %13, %%edx;"
            "cmpl %%eax, %%edx;"
            "je shift_flag_down;" //UBACITI PROVERU DA LI JE CTRL DOWN, ZA CTRL ONLY DOWN

            "shift_flag_down:"

            "movl %14, %%edx;"
            "cmpl %%ebx, %%edx"
            "je shift_and_ctrl_down;"

            ""//UBACITI UPIS NA BUFFER U ZAVISNOSTI OD IZABRANOG SC-A



            "shift_and_ctrl_down:"
            ""
            //"pushl %%eax;" //Storing shift/ctrl/alt flags
            //"pushl %%ebx;"
            //"pushl %%ecx;"

            //"end_of_file:"//TO BE USED WHEN FILE REACHED 400

            "end:"

            : "=a" (shift_flag), "=b" (ctrl_flag), "=c" (alt_flag)
            : "a" (shift_flag), "b"(ctrl_flag), "c"(alt_flag), "g" (scancode),
              "g" (200), "g" (300), "g" (201), "g" (301), "g" (202), "g" (302), "g" (1), "g" (0), "g" (400)
            : "%edx", "memory"
            );


    char C[5];
    itoa(alt_flag, C);
    write_new_line();
    printstr(C);

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


