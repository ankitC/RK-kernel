#include <stdio.h>
#include <asm/unistd.h>
//#include <linux/syscalls.h>
#define __NR_calc 376

__syscall3(long, calc, long, long, char)

int main(void)
{
	calc(3,4,'+');
	printf("Calculator!\n");
	return 0;
}
