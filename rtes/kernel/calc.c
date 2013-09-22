#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <errno.h>

/*asmlinkage int sys_calc(long first, long second, char operation){

  printk(KERN_DEBUG "TestCalc\n");
  }*/


SYSCALL_DEFINE3(calc, long, first, long, second, char, operation)
{
	long result = 0;

	switch(operation){

		case '+':
			result = first + second;
			break;
		case '-':
			result = first - second;
			break;
		case '*':
			result = first * second;
			break;
		case '/':
			if (second == 0)
				errno = EDOM;
			result = first / second;
			break;
		default:
			printk(KERN_WARNING "Operation not Supported.\n");
			break;
	}

	printk(KERN_INFO "First Operand = %ld\nSecond Operand = %ld\n \
			Operation = %c\nResult = %ld\n", first, second, operation, result);
	return result;
}
