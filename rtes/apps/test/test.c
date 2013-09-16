//A simple Hello world program
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void)
{
	int fd2 = open ("/sdcard/t.txt", O_RDWR|O_CREAT);
	printf("test, world! I am an Android app. %d\n", fd2);
	exit(0);
	return 0;
}
