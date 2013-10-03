#include <stdio.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <linux/types.h>
#include <unistd.h>
#include <linux/time.h>

int main(int argc, char* argv[])
{

	pid_t pid = (pid_t)atoi(argv[1]);


	syscall(__NR_cancel_reserve, pid );
	return 1;
}
