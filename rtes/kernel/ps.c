#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define LINELENGTH 43
#define BUFF_SIZE(x) (x * LINELENGTH + 1)



SYSCALL_DEFINE0(count_processes)
{
	unsigned short num_processes = 0;
	struct task_struct *task;
	read_lock(&tasklist_lock);
	for_each_process (task)
		num_processes++;
	read_unlock(&tasklist_lock);
	return num_processes;
}

SYSCALL_DEFINE2(list_processes, char*, user_buffer, int, len)
{
	static char *kernel_buffer;
	struct task_struct *task;
	unsigned long bytes_remaining = 0;
	char* null_char= "\0";
	int num_processes = 0;
	int kernel_buffer_len;

	read_lock(&tasklist_lock);
	for_each_process (task)
		num_processes++;

	if((kernel_buffer=kmalloc( BUFF_SIZE(num_processes), \
			GFP_KERNEL)) == NULL)
	{
		printk( KERN_DEBUG "Kmalloc error.\n");
		return -1;
	}

	kernel_buffer_len = num_processes * LINELENGTH;
	sprintf(kernel_buffer,"pid\tpr\tname\n");

	for_each_process (task)
	{
		sprintf(kernel_buffer, "%s %i\t%i\t%s\n", kernel_buffer, task->pid,\
			   	task->prio, task->comm);
	}
	read_unlock(&tasklist_lock);

	sprintf(kernel_buffer,"%s%s", kernel_buffer, null_char);

	kernel_buffer_len = strlen(kernel_buffer);
	bytes_remaining = copy_to_user(user_buffer, kernel_buffer, \
				len < kernel_buffer_len ? len : kernel_buffer_len );
	kfree(kernel_buffer);

	return bytes_remaining;
}
