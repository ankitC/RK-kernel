#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <sys/syscall.h>

#define LINELENGTH 43
#define BUFF_SIZE(x) (x * LINELENGTH + 1)


static sigset_t mask;
static int exit_process = 0;

int list_processes(char* buffer, int len)
{
	int retval = 0;
	if ((retval = syscall(__NR_list_processes, buffer, len)) == 0)
	{
		printf("return %d\n", retval);
		return 0;
	}
	else
		return -1;
}

int count_processes()
{
	return syscall(__NR_count_processes);
}

void sig_int_handler()
{
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	exit_process = 1;
	return;
}

int main(void)
{
	sigemptyset( &mask );
	sigaddset(&mask, SIGTSTP);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	signal(SIGINT, sig_int_handler);

	while(!exit_process)
	{
		int i = (int) count_processes();
		char* buffer = calloc( BUFF_SIZE(i), 1);

		if (!list_processes(buffer, BUFF_SIZE(i)))
		{
			printf("%s",buffer);
		}

		free(buffer);

		sleep(2);
	}

}
