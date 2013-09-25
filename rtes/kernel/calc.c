/*
 * Kerenel program to do simple 
 * mathematical calculations
 */

#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#define FIXED_SCALE 10000
#define SATURATE 0x7FFFFFFF
#define ERROR 0XDEADBEEF

SYSCALL_DEFINE3(calc, long, first, long, second, char, operation)
{
	long result = 0;

	printk(KERN_INFO "FIRST = %ld\n", first);
	printk(KERN_INFO "SECOND = %ld\n", second);

	switch(operation){

		case '+':
			if (first == SATURATE || second == SATURATE){
				result = ERROR;
				break;
			}
			result = first + second;
			break;
		case '-':
			if (first == SATURATE || second == SATURATE){
				result = ERROR;
				break;
			}
			result = first - second;
			break;
		case '*':
			if (first == SATURATE || second == SATURATE){
				result = ERROR;
				break;
			}
			result = first * second;
			break;
		case '/':
			first =  first * FIXED_SCALE;
			printk(KERN_INFO "FIRST = %ld\n", first);
			if (second == 0){
				result = ERROR;
				break;
			}
			printk(KERN_INFO "SECOND = %ld\n", second);
			result = first / second;
			printk(KERN_INFO "RESULT = %ld\n", result);
			break;
		default:
			printk(KERN_WARNING "Operation not Supported.\n");
			break;
	}

	printk(KERN_INFO "First Operand = %ld\nSecond Operand = %ld\n\
			Operation = %c\nResult = %ld\n", first, second, operation, result);
	return result;
}
