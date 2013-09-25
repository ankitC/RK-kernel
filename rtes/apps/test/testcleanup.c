#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

void main(){
int fd1, fd2, fd3;

char* file1="/sdcard/t.txt";
char* file2="/sdcard/t1.txt";
char* file3="/sdcard/t2.txt";

fd1=open(file1,O_RDONLY);
fd2=open(file2,O_RDONLY);
fd3=open(file3,O_RDONLY);

printf("fd1=%d fd2=%d fd3=%d\n",fd1,fd2,fd3);
close(fd2);
exit(2);
}
