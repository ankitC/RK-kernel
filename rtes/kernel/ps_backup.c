#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/nodefuncs.h>
#include <asm/uaccess.h>

#define LINELENGTH 43
#define BUFF_SIZE(x) (x * LINELENGTH + 1)
#define D(x) x

/*
 * Iterates through the existing processes in the system
 * and returns the total number of processes
 */
unsigned int do_count_processes(void)
{
	unsigned int num_processes = 0;
	struct task_struct *task;

	read_lock(&tasklist_lock);
	for_each_process (task)
		num_processes++;
	read_unlock(&tasklist_lock);

	return num_processes;
}

/*
 * The syscall copies the PIDs, priorities and names of the
 * currently running processes to user space memory.
 */
unsigned long do_list_processes(char* user_buffer, int len)
{
	static char *kernel_buffer = NULL;
	TASK_NODE* list = NULL;
	struct task_struct *task = NULL;
	unsigned long bytes_remaining = 0;
	char* null_char= "\0";
	int num_processes = 0;
	int kernel_buffer_len = 0;
	TASK_NODE* curr = NULL;
	list=NULL;

	//Locking the data structure
	read_lock(&tasklist_lock);

	for_each_process (task)
	{
		curr = kmalloc( sizeof(TASK_NODE), GFP_KERNEL);
		curr->pid = task->pid;
		curr->prio = task->prio;
		curr->next = NULL;
		strcpy( curr->name, task->comm);
		add_node(&list, curr);
		num_processes++;
	}

	read_unlock(&tasklist_lock);
	//End of data structure access

	if((kernel_buffer=kmalloc( BUFF_SIZE(num_processes), \
			GFP_KERNEL)) == NULL)
	{
		printk( KERN_DEBUG "Kmalloc error.\n");
		return -1;
	}


	sprintf(kernel_buffer,"pid\tpr\tname\n");


	curr = list;

	while(curr->next!=NULL)
	{
		sprintf(kernel_buffer, "%s %i\t%i\t%s\n", kernel_buffer, curr->pid,\
				curr->prio, curr->name);
		curr=curr->next;
	}

	D(printk(KERN_INFO "Made the message\n"));
	delete_linked_list(&list);
	D(printk(KERN_INFO "finished deleting\n"));
	
	sprintf(kernel_buffer,"%s%s", kernel_buffer, null_char);
	kernel_buffer_len = strlen(kernel_buffer);

	bytes_remaining = copy_to_user(user_buffer, kernel_buffer, \
				len < kernel_buffer_len ? len : kernel_buffer_len );

	kfree(kernel_buffer);

	return bytes_remaining;
}

/*
 *System call definition for count processes
 */
SYSCALL_DEFINE0(count_processes)
{
	return do_count_processes();
}

/*
 *System call definition for list processes
 */
SYSCALL_DEFINE2(list_processes, char*, user_buffer, int, len)
{
	return do_list_processes(user_buffer, len);
}
