#include <string.h>
#include <unistd.h>

#include "scan.h"

#define UTIL_IMPLEMENTATION
#include "../utils.h"

#define BUFFER_SIZE 128


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



    _exit(0);
	/* ucitavanje podesavanja */

	/* ponavljamo: */
		/* ucitavanje i otvaranje test fajla */
		/* parsiranje fajla, obrada scanecodova, ispis */

}
