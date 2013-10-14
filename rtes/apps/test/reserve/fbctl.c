#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>                  /*  for sleep()  */
#include <asm/unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#define D(x)

static int busy = 7;
static int end_count = 0;
static int n = 8;

static void waitfor(int nMilliseconds)
{

	time_t start, end;
	start = time(0);
	end = start;
	while (((end - start) < nMilliseconds) && (end_count == 1))
		end = time(0);
	printf("N=%d\n",n);
	sleep(10-busy);
	printf("Worked for %d seconds.\nSlept for %d seconds.", end - start, 10 - busy);
}

static void sig_excess_handler()
{
	busy = busy - 1;
	end_count = 0;
	printf("Caught SIGEXCESS. Busy=%d\n", busy);
	return;
}

void *util()
{


	int ten =10;
	time_t start, end;
	while (ten --)
	{
		printf("Self id %d\n n=%d", pthread_self(), ten);
		start = time(0);
		end = start;
		while (((end - start) < 5))
		end = time(0);
	
		//waitfor(5);
		sleep(5);
	}

/*	printf("Self id %d\n", pthread_self());
	int i =0;
	while(n){
		end_count = 1;
		waitfor(busy);
		i++;
		if (i ==18)
			break;
	}
*/	
}
/*Application for displaying current processes*/
int main(void) {

	pid_t pid = getpid();
	pthread_t p[3];
	int j = 0;
	for (j = 0; j < 3; j++)
	{
		pthread_create(&p[j], NULL, util, NULL) ;
	}

	printf("In user pid=%u\n", pid);
	struct timespec ctime;
	ctime.tv_sec = 4;
	ctime.tv_nsec = 0; /*25ms*/

	struct timespec ttime;
	ttime.tv_sec = 10;
	ttime.tv_nsec = 0; /*100ms*/

	unsigned int prio = 120;

	if(!syscall(__NR_set_reserve, pid, ctime, ttime, prio ))
		printf("in back user\n");


	signal(SIGEXCESS, sig_excess_handler);

	for (j = 0; j < 3; j++)
	{
		pthread_join(p[j],NULL ) ;
	}


exit(2);
}
