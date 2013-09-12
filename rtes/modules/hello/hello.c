//Sample LKM
#include <linux/kernel.h>
#include <linux/module.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>

int init_module(void){

	printk(KERN_INFO "Hello, world! I am a process %u with priority %d\n", \
			getpid(), getpriority(PRIO_PROCESS,0);
	return 0;

}

void cleanup_module(void){

}
