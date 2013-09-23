#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>                  /*  for sleep()  */
#include <asm/unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <curses.h>
#include <sys/syscall.h>

#define LINELENGTH 43
#define BUFF_SIZE(x) (x * LINELENGTH + 1)

#define D(x)


static sigset_t mask;
static int exit_process = 0;

int list_processes(char* buffer, int len)
{
	int retval = 0;
	if ((retval = syscall(__NR_list_processes, buffer, len)) == 0)
		return 0;
	else
		return -1;
}

int count_processes()
{
	return syscall(__NR_count_processes);
}

void sig_int_handler()
{
	exit_process = 1;
	return;
}

/*Application for displaying current processes*/
int main(void) {

	WINDOW * mainwin;

	/*Blocking signals to only service CLTR-C*/
	sigemptyset( &mask );
	sigaddset(&mask, SIGTSTP);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	signal(SIGINT, sig_int_handler);
	
	int i = (int) count_processes();
	char* buffer = calloc( BUFF_SIZE(i), 1);

	/*  Initialize ncurses  */
	if ( (mainwin = initscr()) == NULL ) {
		fprintf(stderr, "Error initialising ncurses.\n");
		exit(EXIT_FAILURE);
	}
	
	while(!exit_process)
	{
		if (!list_processes(buffer, BUFF_SIZE(i)))
		{
			printw("%s",buffer);
		}

		refresh();
		sleep(1);
		clear();
	}

	/*Clean up after ourselves*/
	free(buffer);
	delwin(mainwin);
	endwin();
	return EXIT_SUCCESS;
}