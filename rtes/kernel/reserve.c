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
#include <linux/sysfs_func.h>

#define D(x) x


/*
 * Introduces the process with the given pid in
 * the reservation framework
 */
static int n = 0;
unsigned int do_set_reserve(pid_t pid, struct timespec C, struct timespec T,\
		unsigned int rt_priority)
{
	struct task_struct *task = NULL, *task_found = NULL;

	unsigned long flags;	
	ktime_t ktime;

	if (n == 0)
	n++;

	printk(KERN_INFO "in set reserve\n");
	if (pid == 0)
	{
		task = current;
		task_found = current;
	}
	else
	{
		read_lock(&tasklist_lock);
		for_each_process (task)
		{
			if (task->pid == pid)
			{
				printk(KERN_INFO "found task\n");
				task_found = task;
				break;
			}
		}
		read_unlock(&tasklist_lock);
	}

	if (!task_found)
			return -1;

	if (task->under_reservation)
		cleanup_hrtimer(&task->reserve_process.hr_timer);
	if (n == 1)
	{
		task->reserve_process.prev_setime = task->se.sum_exec_runtime;
		create_directories();
	}

	ktime = ktime_set(C.tv_sec, C.tv_nsec);
	spin_lock_irqsave(&task->reserve_process.reserve_spinlock, flags);

/* Set running according to its current status */
	if (task == current)
		task->reserve_process.running = 1;
	else
		task->reserve_process.running = 0;

	ktime = ktime_set( C.tv_sec, C.tv_nsec);

	strcpy(task->reserve_process.name, "group11");
	task->reserve_process.pid = task->pid;
	task->reserve_process.monitored_process = task;
	task->reserve_process.signal_sent = 0;
	task->reserve_process.buffer_overflow = 0;
	task->reserve_process.C = C;
	task->reserve_process.T = T;
	task->reserve_process.remaining_C = ktime;
	task->reserve_process.prev_setime = task->se.sum_exec_runtime;
	task->reserve_process.spent_budget.tv_sec = 0;
	task->reserve_process.spent_budget.tv_nsec = 0;
	task->under_reservation = 1;
	task->reserve_process.c_buf.start = 0;
	task->reserve_process.c_buf.read_count = 0;
	task->reserve_process.c_buf.buffer[0] = 0;
	task->reserve_process.c_buf.end = 0;
	task->reserve_process.ctx_buf.start = 0;
	task->reserve_process.ctx_buf.read_count = 0;
	task->reserve_process.ctx_buf.buffer[0] = 0;
	task->reserve_process.ctx_buf.end = 0;

	init_hrtimer(&task->reserve_process);

	spin_unlock_irqrestore(&task->reserve_process.reserve_spinlock, flags);

	create_pid_dir_and_reserve_file (task);
	printk(KERN_INFO "set all reserves pid=%u\n", task->pid);
	return 0;
}

/*
 * Removes the process with the pid from the reservation framework
 */
unsigned long do_cancel_reserve(pid_t pid)
{
	struct task_struct *task = NULL, *task_found = NULL;

	if (pid == 0)
	{
		task = current;
		task_found = current;
	}
	else
	{
		read_lock(&tasklist_lock);
		for_each_process (task)
		{
			if (task->pid == pid)
			{

				task_found = task;
				break;
			}
		}
		read_unlock(&tasklist_lock);
	}

	if (!task_found)
		return -1;
	if (task->under_reservation)
	{
		/*if (task->tgid != task->pid)
		{
			task = current->group_leader;
		}*/
		cleanup_hrtimer(&task->reserve_process.hr_timer);
		task->under_reservation = 0;
		return 0;
	}
	printk(KERN_INFO "Task is not under reservation\n");
	return 0;
}

/*
 * Suspends the current running job
 */
unsigned long do_end_job()
{
	struct task_struct *task = NULL;

	task = current;
	if (task->under_reservation)
	{
		task->reserve_process.signal_sent = 1;
		/** Invoke another function **/
	}

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
/*
 *System call definition for end_job
 */
SYSCALL_DEFINE0(end_job)
{
	return do_end_job();
}
