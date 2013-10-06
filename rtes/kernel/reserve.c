#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <asm/spinlock.h>
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
	struct task_struct *task = NULL;
	unsigned long flags;
	printk(KERN_INFO "in set resrve\n");
	if (pid == 0)
	{
		if (current->tgid != current->pid)
		{
			task = current->group_leader;
		}
		else
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
				if (task->tgid != task->pid)
				{
					task = task->group_leader;
				}
				break;
			}
		}
		read_unlock(&tasklist_lock);

		if (!task)
			return -1;
	}

	if (task->under_reservation)
		cleanup_hrtimer(&task->reserve_process.hr_timer);


	spin_lock_irqsave(&task->reserve_process.reserve_spinlock, flags);
	strcpy(task->reserve_process.name, "gruop11");

	task->under_reservation = 1;
	task->reserve_process.pid = pid;
	task->reserve_process.monitored_process = task;
	task->reserve_process.signal_sent = 0;
	task->reserve_process.C = C;
	task->reserve_process.T = T;
	task->reserve_process.spent_budget = C;
	init_hrtimer(&task->reserve_process);
	spin_unlock_irqrestore(&task->reserve_process.reserve_spinlock, flags);
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

	cleanup_hrtimer(&task->reserve_process.hr_timer);
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
