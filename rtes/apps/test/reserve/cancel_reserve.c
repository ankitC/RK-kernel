#include <stdio.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <linux/types.h>
#include <unistd.h>
#include <linux/time.h>

void cancel_reserve(pid_t pid)
{
	if (syscall(__NR_cancel_reserve, pid) < 0)
		printf("Error: Cancel reserve failed\n");
}


int main(int argc, char* argv[])
{

	pid_t pid = (pid_t)atoi(argv[1]);

	cancel_reserve(pid );
	return 1;
}
