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

#define nanos(x) (x*1000000000)
/*
 * Function busy loops for given time
 */
void waitfor(int seconds)
{
	time_t start, end;
	start = time(0);
	end = start;
	while ((end - start) < seconds)
		end = time(0);
}

/*
 * Function to calculte different in time for two structs of timeval
 */
long long
timeval_diff(struct timeval *end_time,
		struct timeval *start_time)
{
	struct timeval* difference;

	difference->tv_sec =end_time->tv_sec -start_time->tv_sec ;
	difference->tv_usec=end_time->tv_usec-start_time->tv_usec;

	/* Using while instead of if below makes the code slightly more robust. */

	while(difference->tv_usec<0)
	{
		difference->tv_usec+=1000000;
		difference->tv_sec -=1;
	}

	return (1000000LL*difference->tv_sec+difference->tv_usec);

}

/*
 * Function waits for timediff microseconds
 */
void wait_for(long long timediff){


	struct timeval starttime, currtime;
	long difference = 1;
	long long timespent = 0;
	gettimeofday(&starttime, 0x00);

	while(timespent < timediff){
		gettimeofday(&currtime, 0x00);
		timespent = timeval_diff(&currtime,&starttime);
	}

}

int main(char **argc, char** argv)
{
	long long CR = 0, TR = 0, CW = 0, TW = 0;

	long long CRns = 0, TRns = 0, CWns = 0, TWns = 0;	
	CR = atoll(argv[1]);
	TR = atoll(argv[2]);
	CW = atoll(argv[3]);
	TW = atoll(argv[4]);

	CRns = atoll(argv[5]);
	TRns = atoll(argv[6]);
	CWns = atoll(argv[7]);
	TWns = atoll(argv[8]);


	pid_t pid = getpid();

	printf("In user pid = %d CR=%lld TR=%lld CW=%lld TW=%lld\n", pid, CR, TR, CW, TW);
	printf("CRns=%lld TRns=%lld CWns=%lld TWns=%lld\n", CRns, TRns, CWns, TWns);
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

	while(1){

		wait_for((nanos(CW)+CWns)/1000);
		usleep((useconds_t)(((nanos(TW)+TWns)-nanos(CW)+CWns)/1000) );
		printf("n= %d \n",n);
	}

	syscall(__NR_cancel_reserve, pid );
	printf("Cancelled Reservation\n");
	return 1;
}

