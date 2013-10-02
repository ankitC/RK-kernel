#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/current.h>
#include <linux/reserve_framework.h>
#include <linux/hr_timer_func.h>

#define D(x) x

/*
 * Introduces the process with the given pid in
 * the reservation framework
 */
unsigned int do_set_reserve(pid_t pid, struct timespec C, struct timespec T,\
		unsigned int rt_priority)
{
	printk(KERN_INFO "in set resrve\n");
	struct task_struct *task;

	if (pid == 0)
	{
		task = current;
	}
	else
	{

		read_lock(&tasklist_lock);
		for_each_process (task)
		{
			if (task->pid == pid)
			{
				printk(KERN_INFO "found task\n");
				break;
			}
		}
		read_unlock(&tasklist_lock);

	}
	
	task->reserve_process->C = C;
	task->reserve_process->T = T;
	task->reserve_process->spent_budget.tv_sec = 0;
	task->reserve_process->spent_budget.tv_nsec = 0;
	task->reserve_process->hr_timer = *(init_hrtimer(T));


	printk(KERN_INFO "set all reserves pid=%u\n", pid);
	return 0;
}

/*
 * Removes the process with the pid from the reservation framework
 */
unsigned long do_cancel_reserve(pid_t pid)
{
	struct task_struct *task;

	if (pid == 0)
	{
		task = current;
	}
	else
	{
		read_lock(&tasklist_lock);
		for_each_process (task)
		{
			if (task->pid == pid)
			{
				break;
			}
		}
		read_unlock(&tasklist_lock);
	}

	cleanup_hrtimer(&task->reserve_process->hr_timer);
	task->reserve_process = NULL;
	return 0;
}

/*
 *System call definition set_reserve
 */
SYSCALL_DEFINE4(set_reserve, pid_t, pid, struct timespec, C, struct timespec, T\
		, unsigned int, rt_priority)
{
	return do_set_reserve(pid, C, T, rt_priority);
}

/*
 *System call definition for cancel_reserve
 */
SYSCALL_DEFINE1(cancel_reserve, pid_t, pid)
{
	return do_cancel_reserve(pid);
}
