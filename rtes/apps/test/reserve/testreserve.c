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

	struct timespec ctime;
	ctime.tv_sec = 0;
	ctime.tv_nsec = 25000000; /*25ms*/

	struct timespec ttime;
	ttime.tv_sec = 0;
	ttime.tv_nsec = 100000000; /*100ms*/

	unsigned int prio = 120;

	//syscall(__NR_set_reserve, pid, ctime, ttime, prio );

	while(1){
		waitfor(25);
	}
	return 1;
}

