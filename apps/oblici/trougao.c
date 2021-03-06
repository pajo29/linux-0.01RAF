#include <string.h>
#include <unistd.h>

#include "../utils.h"

#define BUFFER_SIZE 128


static int obim;
static int povrsina;

static int a = 0;
static int b = 0;
static int c = 0;

static int provera_unosa(int i, int len);


void unos_trougao()
{
	write(1, "Unesite tri stranice odvojene razmakom za racunanje:\n", strlen("Unesite tri stranice odvojene razmakom za racunanje:\n"));

	char buffer[BUFFER_SIZE];
	int len, i;

	len = read(0, buffer, BUFFER_SIZE);

	a = atoi(buffer);
	
	for(i = 0; i < len; i++)
		if(buffer[i] == ' ') break;

	if(provera_unosa(i, len))
	{
		unos_trougao();	
		return;
	}

	b = atoi(buffer + i + 1);

	for(i = i + 1; i < len; i++)
		if(buffer[i] == ' ') break;

	if(provera_unosa(i, len))
	{
		unos_trougao();	
		return;
	}

	c = atoi(buffer + i + 1);

	char aC[10];
	char bC[10];
	char cC[10];

	len = itoa(a, aC);
	aC[len++] = ' ';
	write(1, "\n\nUspesan unos! Vas unos je: \n", strlen("\n\nUspesan unos! Vas unos je: \n"));
	write(1, aC, len);

	len = itoa(b, bC);
	bC[len++] = ' ';
	write(1, bC, len);

	len = itoa(c, cC);
	write(1, cC, len);

	write(1, "\n\n\n", strlen("\n\n\n"));
}


int obim_trougao()
{
	if(a == 0 || b == 0 || c == 0)
	{
	write(1, "Stranice nisu validne, ponoviti unos.\n", strlen("Stranice nisu validne, ponoviti unos.\n"));
	return 0;
	}

	__asm__(
		"addl %%ebx, %%eax;"
		"addl %%ecx, %%eax;"
		: "=a" (obim)
		: "a" (a), "b" (b), "c" (c)
	);
	
	return obim;
}

int povrsina_trougao()
{
	if(a == 0 || b == 0 || c == 0)
	{
	write(1, "Stranice nisu validne, ponoviti unos.\n", strlen("Stranice nisu validne, ponoviti unos.\n"));
	return 0;
	}


	__asm__(
		"imull %%ebx, %%eax;"
		"movl $0x0, %%edx;"
		"idivl %%ecx;"
		: "=a" (povrsina)
		: "a" (a), "b" (b), "c" (2)
	);


	return povrsina;
}


static int provera_unosa(int i, int len)
{
	if(i == len)
	{
		write(1, "Nepravilan unos. Pokusajte ponovo\n \n", strlen("Nepravilan unos. Pokusajte ponovo\n \n"));
		a = b = c = 0;
		return 1;
	}
	else return 0;
}

