#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
//static char msg[BUF_LEN];	/* The msg the device will give when asked */
//static char *msg_Ptr;

SYSCALL_DEFINE0(count_processes)
{
	unsigned short numProcesses = 0;
	struct task_struct *task;
	read_lock(&tasklist_lock);
	for_each_process (task)
		numProcesses++;
	read_unlock(&tasklist_lock);
	return numProcesses;
}
