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

static char scan_code_first_row[SCAN_CODE_SIZE];
static char scan_code_second_row[SCAN_CODE_SIZE];

static int mnemonic_size = 0;
static char mnemonic_key[SCAN_CODE_SIZE];
static char mnemonic[SCAN_CODE_SIZE][64];

static int shift_flag;
static int ctrl_flag;
static int alt_flag;

static int volatile result_global = 0;
static int volatile alt_flag_global = 0;


void check_for_files(int file_TBL, int file_MN);


void load_config(const char *scancodes_filename, const char *mnemonic_filename)
{
    shift_flag = ctrl_flag = alt_flag = 0;
    println();

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
        scan_code_first_row[i] = buffer_first_line[i];
        scan_code_second_row[i] = buffer_second_line[i];
    }

    char mn_num[3];
    fgets(mn_num, 3, file_MN);

    mnemonic_size = atoi(mn_num);

    for(i = 0; i < mnemonic_size; i++)
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
	int result = result_global = 0;
	int a;


    __asm__ __volatile__(
            "cld;"
            "movl %0, %%eax;"
            "movl $400, %%ebx;"
            "cmpl %%eax, %%ebx;"
            "je end;" //TO BE USED WHEN FILE REACHES 400

            //CTRL/SHIFT/ALT TESTS

            //shift down compare
            "movl %0, %%eax;"
            "movl $200, %%edx;"
            "cmpl %%eax, %%edx;"
            "je shift_down;"

            //shift up compare
            "movl %0, %%eax;"
            "movl $300, %%edx;"
            "cmpl %%eax, %%edx;"
            "je shift_up;"

            //ctrl down compare
            "movl %0, %%eax;"
            "movl $201, %%edx;"
            "cmpl %%eax, %%edx;"
            "je ctrl_down;"

            //ctrl up compare
            "movl %0, %%eax;"
            "movl $301, %%edx;"
            "cmpl %%eax, %%edx;"
            "je ctrl_up;"

            //alt down compare
            "movl %0, %%eax;"
            "movl $202, %%edx;"
            "cmpl %%eax, %%edx;"
            "je alt_down;"

            //alt up compare
            "movl %0, %%eax;"
            "movl $302, %%edx;"
            "cmpl %%eax, %%edx;"
            "je alt_up;"

            "jmp no_flags;"

            //FLAG_JUMPS

            "shift_down:"
            "movl $1, (shift_flag);"
            "jmp end;"

            "shift_up:"
            "movl $0, (shift_flag);"

            "jmp end;"

            "ctrl_down:"
            "movl $1, (ctrl_flag);"
            "jmp end;"

            "ctrl_up:"
            "movl $0, (ctrl_flag);"
            "jmp end;"

            "alt_down:"
            "movl $1, (alt_flag);"
            "jmp end;"

            "alt_up:"
            "movl $0, (alt_flag);"
            "jmp alt_flag_up;"

            "no_flags:"

            "cld;"
            "movl $1, %%edx;"
            "movl (shift_flag), %%eax;"
            "cmpl %%eax, %%edx;"
            "je shift_flag_down;"

            "movl (ctrl_flag), %%eax;"
            "cmpl %%eax, %%edx;"
            "je shift_flag_up_ctrl_flag_down;"

            "movl (alt_flag), %%eax;"
            "cmpl %%eax, %%edx;"
            "je alt_flag_down;"

            //NO FLAGS
            "cld;"
            "lea (scan_code_first_row), %%esi;"
            "add %0, %%esi;"
            "lodsb;"
            "mov %1, %%edi;"
            "stosb;"
            "movl $1, (result_global);"
            "jmp end;"

            "shift_flag_down:"

            "movl $1, %%edx;"

            "movl (alt_flag), %%ebx;"
            "cmpl %%ebx, %%edx;"
            "je alt_shift_down;"

            "movl (ctrl_flag), %%ebx;"
            "cmpl %%ebx, %%edx;"
            "je shift_and_ctrl_down;"

            //UBACITI UPIS NA BUFFER U ZAVISNOSTI OD IZABRANOG SC-A
            "cld;"
            "lea (scan_code_second_row), %%esi;"
            "add %0, %%esi;"
            "lodsb;"
            "mov %1, %%edi;"
            "stosb;"
            "movl $1, (result_global);"
            "jmp end;"

            "found_key:" //MOVES STRING FROM MATRIX
            "mov %1, %%edi;"
            "lea (mnemonic), %%esi;"
            "movl %%ebx, %%eax;"
            "xorl %%edx, %%edx;"
            "movl $64, %%ebx;"
            "imull %%ebx, %%eax;"
            "add %%eax, %%esi;"
            "cld;"
            "xorl %%edx, %%edx;"
            "movb $0, %%bl;"
            "load_to_buff:"
            "lodsb;"
            "stosb;"
            "incl %%edx;"
            "cmpb %%al, %%bl;"
            "jne load_to_buff;"
            "movl %%edx, (result_global);"
            "jmp end;"

            "shift_and_ctrl_down:"
            "cld;"
            "lea (scan_code_second_row), %%esi;"
            "add %0, %%esi;"
            "lodsb;"
            "lea (mnemonic_key), %%esi;"
            "movb %%al, %%ah;"
            "xorl %%ebx, %%ebx;"

            //LOOP TO FINT CTRL
            "movl (mnemonic_size), %%ecx;"
            "key_search:"
            "lodsb;"
            "cmpb %%al, %%ah;"
            "je found_key;"
            "incl %%ebx;"
            "decl %%ecx;"
            "cmpl $0, %%ecx;"
            "jne key_search;"
            "jmp end;"

            "shift_flag_up_ctrl_flag_down:"

            "cld;"
            "lea (scan_code_first_row), %%esi;"
            "add %0, %%esi;"
            "lodsb;"
            "lea (mnemonic_key), %%esi;"
            "movb %%al, %%ah;"
            "xorl %%ebx, %%ebx;"

            //LOOP TO FINT CTRL
            "movl (mnemonic_size), %%ecx;"
            "key_search_2:"
            "lodsb;"
            "cmpb %%al, %%ah;"
            "je found_key;"
            "incl %%ebx;"
            "decl %%ecx;"
            "cmpl $0, %%ecx;"
            "jne key_search_2;"
            "jmp end;"

            "alt_flag_up:"
            "cld;"
            "movl (alt_flag_global), %%eax;"
            "mov %1, %%edi;"
            "stosb;"
            "movl $0, (alt_flag_global);"
            "movl $1, (result_global;)"
            "jmp end;"

            "alt_flag_down:"
            "cld;"
            "lea (scan_code_first_row), %%esi;"
            "add %0, %%esi;"
            "lodsb;"
            "sub $48, %%al;"
            "xor %%ah, %%ah;"
            "pushl %%eax;"
            "movl (alt_flag_global), %%eax;"
            "popl %%ebx;"
            "imull $10, %%eax;"
            "addl %%ebx, %%eax;"
            "movl %%eax, (alt_flag_global);"
            "jmp end;"

            "alt_shift_down:"
            "cld;"
            "lea (scan_code_second_row), %%esi;"
            "add %0, %%esi;"
            "lodsb;"
            "sub $48, %%al;"
            "xor %%ah, %%ah;"
            "pushl %%eax;"
            "movl (alt_flag_global), %%eax;"
            "popl %%ebx;"
            "imull $10, %%eax;"
            "addl %%ebx, %%eax;"
            "movl %%eax, (alt_flag_global);"

            "end:"


            :: "g" (scancode), "g" (buffer)
            : "%edx", "%esi", "%edi", "memory", "%eax", "%ebx", "%ecx"
            );



    result = result_global;
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

    println();
}


