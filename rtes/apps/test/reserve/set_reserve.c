#include <stdio.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <linux/types.h>
#include <unistd.h>
#include <linux/time.h>

int main(int argc, char* argv[])
{

	pid_t pid = (pid_t)atoi(argv[1]);

	printf("In user pid=%u\n", pid);
	struct timespec ctime;
	ctime.tv_sec = 1;
	ctime.tv_nsec = 0; /*25ms*/

	struct timespec ttime;
	ttime.tv_sec = 5;
	ttime.tv_nsec = 0; /*100ms*/

	unsigned int prio = 120;

	syscall(__NR_set_reserve, pid, ctime, ttime, prio );
	return 1;
}
