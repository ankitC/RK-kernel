#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <sys/syscall.h>

/*Wrapper for count processes*/
int count_processes(void)
{
	return syscall(__NR_count_processes);
}

int main(int argc, char** argv)
{
	int numProcesses = 0;
	numProcesses = count_processes();
	printf("Counted Processes: %d\n", numProcesses);
	return 0;
}
