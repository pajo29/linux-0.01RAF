#include <string.h>
#include <unistd.h>

#include "../utils.h"

#define BUFFER_SIZE 128

static int obim;
static int povrsina;

static int a = 0;

void unos_kvadrat()
{
	write(1, "Unesite stranicu za racunanje:\n", strlen("Unesite stranicu za racunanje:\n"));
	
	char buffer[BUFFER_SIZE];

	read(0, buffer, BUFFER_SIZE);

	a = atoi(buffer);

	char aC[10];
	int len = itoa(a, aC);

	write(1, "\n\nUspesan unos! Vas unos je: \n", strlen("\n\nUspesan unos! Vas unos je: \n"));
	write(1, aC, len);	

	write(1, "\n\n\n", strlen("\n\n\n"));
	
}

int obim_kvadrat()
{
	write(1, "Unesite stranicu za racunanje:\n", strlen("Unesite stranicu za racunanje:\n"));
}

int povrsina_kvadrat()
{
	write(1, "Unesite stranicu za racunanje:\n", strlen("Unesite stranicu za racunanje:\n"));
}
