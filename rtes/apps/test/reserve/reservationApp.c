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
	long CRns = 0, TRns = 0, CWns = 0, TWns = 0;	
	CR = atol(argv[1]);
	TR = atol(argv[2]);
	CW = atol(argv[3]);
	TW = atol(argv[4]);

	CRns = atol(argv[5]);
	TRns = atol(argv[6]);
	CWns = atol(argv[7]);
	TWns = atol(argv[8]);


	pid_t pid = getpid();

	printf("In user pid = %u CR=%d TR=%d CW=%d TW=%d\n", pid, CR, TR, CW, TW);
	struct timespec ctime;
	ctime.tv_sec = CR;
	ctime.tv_nsec = CRns; 

	struct timespec ttime;
	ttime.tv_sec = (long) TR;
	ttime.tv_nsec = TRns; 

	unsigned int prio = 120;

	if(!syscall(__NR_set_reserve, pid, ctime, ttime, prio ))
		printf("Reservation Set\n");

	int n = 0;
	long CWval = (CW + CWns/1000000000);
	long TWval = (TW + TWns/1000000000);
	while(n++ < 1000){

		waitfor(CWval);
		sleep(TWval-CWval);
	}

	syscall(__NR_cancel_reserve, pid );
	printf("Cancelled Reservation\n");
	return 1;
}

