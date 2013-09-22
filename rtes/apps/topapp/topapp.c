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
//	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	exit_process = 1;
	return;
}


int main(void) {

	WINDOW * mainwin;
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

	printf("Screen Initialized\n");
	/*  Display "Hello, world!" in the centre of the
	 *		screen, call refresh() to show our changes, and
	 *			sleep() for a few seconds to get the full screen effect  */
/*	while(1){
		mvaddstr(13, 33, "Hello, world!");
		refresh();
		sleep(2);
	}*/

	while(!exit_process)
	{
		if (!list_processes(buffer, BUFF_SIZE(i)))
		{
			printw("%s",buffer);
		}
		refresh();
		sleep(2);
		clear();
	}
	free(buffer);


	/*  Clean up after ourselves  */

	delwin(mainwin);
	endwin();
	refresh();

	return EXIT_SUCCESS;
}
