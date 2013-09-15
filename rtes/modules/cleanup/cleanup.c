#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/unistd.h>
#include <asm/current.h>


MODULE_LICENSE("GPL");

unsigned long **sys_call_table = (unsigned long **) 0xc000ec84;

asmlinkage int (*original_sys_exit)(int);

asmlinkage int our_fake_exit_function(int error_code)
{
	/*print message on console every time we are called*/
	printk("HEY! sys_exit called with error_code=%d\n",error_code);
	
	/*call original sys_exit and return its value*/
	return original_sys_exit(error_code);
}

int init_module(void)
{
	/*store reference to the original sys_exit call*/
//	if (strstr (current->comm, "sh"))
//	{
		original_sys_exit = (void *) sys_call_table[__NR_exit];

		/*manipulate sys_call_table to call our fake exit
	 *     function instead*/
		sys_call_table[__NR_exit] = (unsigned long* ) our_fake_exit_function;
//	}
	return 0;
}

void cleanup_module(void)
{
//	if (strstr (current->comm, "sh"))
//	{
	/*restore original sys_exit*/
	sys_call_table[__NR_exit] = (unsigned long* ) original_sys_exit;
//	}
}
