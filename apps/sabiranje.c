#include <string.h>
#include <unistd.h>

#define MAX 50

int main(int argc, char **argv)
{
	char line[MAX];

	int firstNum = 0;
	int secondNum = 0;

	write(1, "Unesite brojeve odvojene razmakom:\n", strlen("Unesite brojeve odvojene razmakom:\n"));

	read(0, line, MAX);

	int i = 0;
	
	while(line[i] != ' ')
	{
		firstNum = firstNum * 10 + (line[i] - '0');
		i++;
	}
	
	i++;

	while(i < strlen(line)-1)
	{
		secondNum = secondNum * 10 + (line[i] - '0');
		i++;
	}
	
	int res = firstNum+secondNum;
	int printingRes = 0;

	while(res != 0)
	{

	printingRes = printingRes * 10 + (res%10);
	res = res / 10;

	}

	char resString[MAX];

	i = 0;

	while(printingRes != 0)
	{
	resString[i] = (printingRes%10) + '0';
	printingRes = printingRes/10;
	i++;
	}


	resString[i++] = '\n';
	resString[i] = '\0';
	write(1, resString, strlen(resString));	


	_exit(0);
}
