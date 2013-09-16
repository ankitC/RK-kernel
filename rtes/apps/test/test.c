//A simple Hello world program
#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <sys/syscall.h>

int count_processes()
{
	return syscall(__NR_count_processes);
}


int main(void)
{
//	int fd2 = open ("/sdcard/t.txt", O_RDWR|O_CREAT);
//	printf("test, world! I am an Android app. %d\n", fd2);
//	exit(0);
	int i=(int)count_processes();
	printf("The process count is %d\n",i);
	return 0;
}
