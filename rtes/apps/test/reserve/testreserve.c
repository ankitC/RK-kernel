#include <stdio.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <linux/types.h>
#include <unistd.h>
#include <linux/time.h>

void waitfor(int nMilliseconds)
{
	time_t start, end;
	start = time(0);
	end = start;
	while ((end - start) < nMilliseconds)
		end = time(0);
}

int main(char **argc, char** argv)
{

	pid_t pid = getpid();

	printf("In user pid=%u\n", pid);
	struct timespec ctime;
	ctime.tv_sec = 1;
	ctime.tv_nsec = 250000000; /*25ms*/

	struct timespec ttime;
	ttime.tv_sec = 2;
	ttime.tv_nsec = 10000000; /*100ms*/

	unsigned int prio = 120;

	if(!syscall(__NR_set_reserve, pid, ctime, ttime, prio ))
		printf("in back user\n");

int  n = 0;	
	while(n++ < 10){
		int i, result=0;
		for(i=0; i<100000; i++){
			result=result+i;
		}
		waitfor(2000);
	}
	return 1;
}

