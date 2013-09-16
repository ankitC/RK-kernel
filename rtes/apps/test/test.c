//A simple Hello world program
#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <sys/syscall.h>

#define LISTLENGTH 43
int list_processes(char* buffer)
{
	int retval = 0;
	if ((retval = syscall(__NR_list_processes, buffer)) == 0)
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
//	int fd2 = open ("/sdcard/t.txt", O_RDWR|O_CREAT);
//	printf("test, world! I am an Android app. %d\n", fd2);
//	exit(0);
	int i = (int) count_processes();
	char* buffer = calloc( LISTLENGTH, 1);

	if (!list_processes(buffer))
	{
		printf("%s",buffer);
	}

	free(buffer);
	return 0;
}
