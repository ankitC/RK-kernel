//A ps driver
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
	retval = syscall(__NR_list_processes, buffer, len);

		printf("Number of bytes returned= %d\n", retval);
		return retval;
}

int count_processes()
{
	return syscall(__NR_count_processes);
}

int main(void)
{
	//int fd2 = open ("/sdcard/t.txt", O_RDWR|O_CREAT);
	//printf("test, world! I am an Android app. %d\n", fd2);
	int i = (int) count_processes();
	//int i=1;
	char* buffer = calloc( BUFF_SIZE(i), 1);
//	char* buffer=NULL;
//	buffer=130;
//	int i=5;
	int retval=0;
	if ((retval=list_processes(buffer, BUFF_SIZE(i)))>0)
	{
		printf("%s\n",buffer);
		printf("Return value=%d\n",retval);
	}

	free(buffer);
	return 0;
}
