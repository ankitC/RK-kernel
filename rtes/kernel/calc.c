#include <linux/kernel.h>
#include <linux/syscalls.h>
SYSCALL_DEFINE3(calc, long, first, long, second, char, operation)
{
	printk(KERN_DEBUG "Test Calc\n");
}
