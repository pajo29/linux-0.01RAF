#include <string.h>
#include <unistd.h>

#define MAX 100

int main(int argc, char **argv)
{
	char buf[MAX];
	int len;

	len = read(0, buf, MAX);

	write(1, buf, len);
	
	_exit(0);
}
