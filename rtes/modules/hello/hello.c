//Sample LKM
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <asm/current.h>

int init_module(void){

	printk(KERN_INFO "Hello, world! I am a process (PID %i) with (PRIORITY %u)\n", current->pid,  current->rt_priority);
	return 0;

}

void cleanup_module(void){

}
