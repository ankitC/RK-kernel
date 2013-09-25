#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

void main()
{
	int fd=0;
	char* dev="/dev/psdev";
	fd=open(dev, O_RDWR);
	
	printf("fd=%d", fd);
	char* buf = "This is a test string";

	write(fd,buf,strlen(buf));
	lseek(fd, 5, SEEK_CUR);
	close(fd);
}
