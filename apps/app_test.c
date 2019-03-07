#include <string.h>
#include <string.h>

int main(int argc, char **argv)
{
	write(1, "Hello world!\n", strlen("Hello world!\n"));
	
	_exit(0);

}
