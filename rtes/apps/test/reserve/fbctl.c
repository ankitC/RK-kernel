#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>                  /*  for sleep()  */
#include <asm/unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <curses.h>
#include <sys/syscall.h>

#define D(x)

static int busy = 10;
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
	printf("Worked for %d seconds.\nSlept for %d seconds.", end-start, busy-n);
}

static void sig_excess_handler()
{
	busy = busy - 1;
	end_count = 0;
	printf("Caught SIGEXCESS. Busy=%d\n", busy);
	return;
}

/*Application for displaying current processes*/
int main(void) {

	signal(SIGEXCESS, sig_excess_handler);

	while(n){
		end_count = 1;
		waitfor(busy);
	}
}
