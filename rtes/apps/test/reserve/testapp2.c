/*
 * This ffile is the Test
 *
 */
#include <stdio.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <linux/types.h>
#include <unistd.h>
#include <linux/time.h>


void waitfor(int seconds)
{
	time_t start, end;
	start = time(0);
	end = start;
	while ((end - start) < seconds)
		end = time(0);
}

int main(char **argc, char** argv)
{
	long CR = 0, TR = 0, CW = 0, TW = 0;
	CW = atol(argv[1]);
	TW = atol(argv[2]);
	CR = atol(argv[3]);
	TR = atol(argv[4]);

	pid_t pid = getpid();

	printf("In user pid = %u CR=%d TR=%d CW=%d TW=%d\n", pid, CR, TR, CW, TW);
	struct timespec ctime;
	ctime.tv_sec = CR;
	ctime.tv_nsec = 0; 

	struct timespec ttime;
	ttime.tv_sec = (long) TR;
	ttime.tv_nsec = 0; 

	unsigned int prio = 120;

	if(!syscall(__NR_set_reserve, pid, ctime, ttime, prio ))
		printf("Reservation Set\n");

	int n = 0;
	while(n++ < 1000){

		waitfor(CW);
		sleep(TW-CW);
	}

	syscall(__NR_cancel_reserve, pid );
	printf("Cancelled Reservation\n");
	return 1;
}

