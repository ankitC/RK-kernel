#include <stdio.h>
/*
 * Busy loop 
 */
int main(int argc, char* argv[])
{
	printf("In user pid=%u\n", getpid());
	while (1);
	return 1;
}
