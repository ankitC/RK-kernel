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
#include <linux/partition_scheduling.h>
#include <linux/linked_list.h>

#define D(x) x

/*
 * Introduces the process with the given pid in
 * the reservation framework
 */
static int n = 0;
extern void disable_auto_hotplug(void);

static unsigned long long calculate_util(struct task_struct * task)
{
	uint64_t C_var = 0;
	uint64_t T_var = 0;
	uint64_t* C_temp = (uint64_t*)&C_var;
	uint64_t* T_temp = (uint64_t*)&T_var;
	uint32_t remainder= 0, t = 0;

	*C_temp = (timespec_to_ns(&task->reserve_process.C));
	*T_temp = timespec_to_ns(&task->reserve_process.T);
	remainder = do_div(*T_temp, 10000);
	t = *T_temp;
	remainder = do_div(*C_temp, t);

	printk(KERN_INFO "Utilisation for a current task: %llu\n", *C_temp);
	return *C_temp;

}






unsigned int do_set_reserve(pid_t pid, struct timespec C, struct timespec T,\
		unsigned int rt_priority)
{
	struct task_struct *task = NULL, *task_found = NULL;
	int retval = 0;
	unsigned long flags;	
	ktime_t ktime;


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
		cleanup_hrtimer(&task->reserve_process.T_timer);

	if (!n)
	{
	//	disable_auto_hotplug();
		task->reserve_process.prev_setime = task->se.sum_exec_runtime;
	}

	n++;
	ktime = ktime_set(C.tv_sec, C.tv_nsec);
	spin_lock_irqsave(&task->reserve_process.reserve_spinlock, flags);

/* Set running according to its current status */
	if (task == current)
		task->reserve_process.running = 1;
	else
		task->reserve_process.running = 0;

	ktime = ktime_set( C.tv_sec, C.tv_nsec);

	strcpy(task->reserve_process.name, "group11");
	task->reserve_process.C = C;
	task->reserve_process.T = T;
	task->reserve_process.U = calculate_util(task);
	task->reserve_process.host_cpu = smp_processor_id();
	retval = admission_test(task);

	if(retval < 0)
	{
		printk(KERN_INFO "Reservation failed pid=%u\n", task->pid);
		spin_unlock_irqrestore(&task->reserve_process.reserve_spinlock, flags);
		return 1;
	}
	task->reserve_process.pid = task->pid;
	task->reserve_process.monitored_process = task;
	task->reserve_process.buffer_overflow = 0;
	task->reserve_process.t_timer_started = 0;
	task->reserve_process.need_resched = 0;
	task->reserve_process.t_timer_started = 0;
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
	spin_unlock_irqrestore(&task->reserve_process.reserve_spinlock, flags);

	set_cpu_for_task(task);
	create_pid_dir_and_reserve_file (task);
	printk(KERN_INFO "Reservation succeeded pid=%u\n", task->pid);
	return retval;
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
		cleanup_hrtimer(&task->reserve_process.T_timer);
		task->under_reservation = 0;
		delete_node(task);
		return 0;
	}
	printk(KERN_INFO "Task is not under reservation\n");
	return 0;
}

/*
 * Suspends the current running job
 */
unsigned long do_end_job(void)
{
	/* Requesting the scheduler to deactivate the current task */
	if (current->under_reservation)
	{
		printk("Ending Job\n");
		current->reserve_process.need_resched = 1;
		set_tsk_need_resched(current);
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
