//A simple Hello world program
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <sys/syscall.h>

#define LINELENGTH 43
#define BUFF_SIZE(x) (x * LINELENGTH + 1)

int list_processes(char* buffer, int len)
{
	int retval = 0;
	if ((retval = syscall(__NR_list_processes, buffer, len)) == 0)
	{
		printf("return %d\n", retval);
		return 0;
	}
	else
		return -1;
}

int count_processes()
{
	return syscall(__NR_count_processes);
}

int main(void)
{
	int fd2 = open ("/sdcard/t.txt", O_RDWR|O_CREAT);
	printf("test, world! I am an Android app. %d\n", fd2);
	exit(0);
/*	int i = (int) count_processes();
	char* buffer = calloc( BUFF_SIZE(i), 1);

	if (!list_processes(buffer, BUFF_SIZE(i)))
	{
		printf("%s",buffer);
	}

	free(buffer);*/
	return 0;
}
