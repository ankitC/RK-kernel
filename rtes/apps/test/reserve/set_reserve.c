#include <stdio.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <linux/types.h>
#include <unistd.h>
#include <linux/time.h>

void set_reserve(pid_t pid, struct timespec C, struct timespec T, unsigned int prio)
{
	if (syscall(__NR_set_reserve, pid, C, T, prio ) < 0)
		printf("Error: Set reserve failed\n");
}



int main(int argc, char* argv[])
{

	pid_t pid = (pid_t)atoi(argv[1]);

	printf("In user pid=%u\n", pid);
	struct timespec ctime;
	ctime.tv_sec = atoll(argv[2]);
	ctime.tv_nsec = 0; /*25ms*/

	struct timespec ttime;
	ttime.tv_sec = atoll(argv[3]);
	ttime.tv_nsec = 0; /*100ms*/

	unsigned int prio = 120;
	set_reserve( pid, ctime, ttime, prio );
	return 1;
}
