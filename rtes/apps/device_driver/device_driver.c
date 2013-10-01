//A simple Hello world program
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <sys/syscall.h>

#define LINELENGTH 43
#define BUFF_SIZE(x) (x * LINELENGTH + 1)

int main(void)
{
	char buffer1[30] = "";
	char buffer2[30] = "";
	int one = open ("/dev/psdev", O_CREAT | O_RDWR);
	int two = open ("/dev/psdev", O_CREAT | O_RDWR);
	if (two < 0)
	{
		printf("invalid");
	}
	else {
	read(one, buffer1, 30);
	read(two, buffer2, 30);
	}
	printf("%s", buffer1);
	printf("%s", buffer2);
	close(one);
	close(two);

	exit(24);
}
