/* modules/hello.c:
 * Simple "hello world!" loadable kernel module
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/current.h>
#include <asm/uaccess.h>	/* for put_user */

#define DRIVER_AUTHOR "Team_11"
#define DRIVER_DESC   "Hello World Kernel module"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

int init_module(void)
{
	printk(KERN_INFO "Hello, world! I am a process (PID %i) with (PRIORITY %u)\
			\n", current->pid,  current->prio);
	return 0;
}

void cleanup_module(void)
{

}
