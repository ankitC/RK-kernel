#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/hr_timer_func.h>
#include <linux/bin_packing.h>
#include <linux/partition_scheduling.h>
#include <asm/current.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/energy_saving.h>

DEFINE_MUTEX (suspend_mutex);
extern spinlock_t(bin_spinlock);

extern BIN_NODE* bin_head;
extern volatile int suspend_processes;
extern int suspend_all;
extern int disable_cpus;
extern spinlock_t bin_spinlock;
/*
 * Waking up suspended tasks
 */

void wakeup_tasks(void)
{
	ktime_t ktime;
	BIN_NODE *curr = bin_head;

	unsigned long flags = 0;

	while (curr)
	{
		if (curr->task->reserve_process->deactivated == 1)
			set_cpu_for_task(curr->task);
		curr = curr->next;
	}

	spin_lock_irqsave(&bin_spinlock, flags);
	curr = bin_head;
	while (curr)
	{
		if (curr->task->reserve_process->deactivated == 1)
		{

			printk(KERN_INFO "%d->Waking up.\n", curr->task->pid);
			ktime = ktime_set(curr->task->reserve_process->C.tv_sec, curr->task->reserve_process->C.tv_nsec);
			curr->task->reserve_process->spent_budget.tv_sec = 0;
			curr->task->reserve_process->spent_budget.tv_nsec = 0;
			curr->task->reserve_process->remaining_C = ktime;
			curr->task->reserve_process->t_timer_started = 0;
			curr->task->reserve_process->deactivated = 0;
			curr->task->reserve_process->need_resched = 0;
			curr->task->reserve_process->pending = 0;
			if(!wake_up_process(curr->task))
				printk(KERN_INFO "Couldn't wake up process %d\n", curr->task->pid);
		}
		curr = curr->next;

	}
	spin_unlock_irqrestore(&bin_spinlock, flags);
	printk(KERN_INFO "All tasks migrated and woken up.\n");
}

/*
 * Suspend tasks from the utilization liked lists
 * When migrate is one and the policy is changed
 */
void migrate_and_start(struct task_struct *task)
{
	BIN_NODE* curr = bin_head;
	unsigned long flags = 0;
	int send_wakeup_msg = 0;

	spin_lock_irqsave(&bin_spinlock, flags);
	while (curr)
	{

		if (curr->task->pid != task->pid)
		{
			/*			if (curr->task->reserve_process->host_cpu != curr->task->reserve_process->prev_cpu)
						{
						curr->task->reserve_process->pending = 1;
						set_tsk_need_resched(curr->task);
						bypass = 0;
						}*/
			curr->task->reserve_process->pending = 1;
			set_tsk_need_resched(curr->task);
		}
		curr = curr->next;
	}
	spin_unlock_irqrestore(&bin_spinlock, flags);

	mutex_lock(&suspend_mutex);
	suspend_processes = 1;
	mutex_unlock(&suspend_mutex);
	printk(KERN_INFO "Suspending all tasks.\n");

	while (1)
	{
		send_wakeup_msg = 0;

		curr = bin_head;

		spin_lock_irqsave(&bin_spinlock, flags);
		while(curr)
		{
			if (curr->task != task)
			{
				if (curr->task->reserve_process->pending == 1)
				{
					if (curr->task->reserve_process->deactivated != 1)
						send_wakeup_msg++;
				}
			}
			curr = curr->next;
		}
		spin_unlock_irqrestore(&bin_spinlock, flags);

		if(send_wakeup_msg == 0)
		{
			mutex_lock(&suspend_mutex);
			if(suspend_processes == 1)
			{
				suspend_processes = 0;
				mutex_unlock(&suspend_mutex);
				break;
			}
			mutex_unlock(&suspend_mutex);
		}
	}
	printk(KERN_INFO "Migrating and waking up tasks.\n");

	wakeup_tasks();

	if (disable_cpus)
	{
		energy_savings();
	}
}

void migrate_only(void)
{
	BIN_NODE* curr = bin_head;
	unsigned long flags = 0;


	if (bin_head == NULL)
		return;

	spin_lock_irqsave(&bin_spinlock, flags);
	while (curr)
	{
		curr->task->reserve_process->pending = 1;
		set_tsk_need_resched(curr->task);
		curr = curr->next;
	}
	spin_unlock_irqrestore(&bin_spinlock, flags);

	mutex_lock(&suspend_mutex);
	suspend_all = 1;
	mutex_unlock(&suspend_mutex);

	printk(KERN_INFO "All tasks set to pending.\n");
}

static struct task_struct *find_process_by_pid(pid_t pid)
{
	return pid ? find_task_by_vpid(pid) : current;
}

/* Setting affinity to CPU */
long reserve_sched_setaffinity(pid_t pid, const struct cpumask *in_mask)
{
	cpumask_var_t cpus_allowed, new_mask;
	struct task_struct *p;
	int retval;

	get_online_cpus();
	rcu_read_lock();

	p = find_process_by_pid(pid);
	if (!p) {
		rcu_read_unlock();
		put_online_cpus();
		return -ESRCH;
	}

	/* Prevent p going away */
	get_task_struct(p);
	rcu_read_unlock();

	if (!alloc_cpumask_var(&cpus_allowed, GFP_KERNEL)) {
		retval = -ENOMEM;
		goto out_put_task;
	}
	if (!alloc_cpumask_var(&new_mask, GFP_KERNEL)) {
		retval = -ENOMEM;
		goto out_free_cpus_allowed;
	}
	retval = -EPERM;

	cpuset_cpus_allowed(p, cpus_allowed);
	cpumask_and(new_mask, in_mask, cpus_allowed);
again:
	retval = set_cpus_allowed_ptr(p, new_mask);

	if (!retval) {
		cpuset_cpus_allowed(p, cpus_allowed);
		if (!cpumask_subset(new_mask, cpus_allowed)) {
			/*
			 * We must have raced with a concurrent cpuset
			 * update. Just reset the cpus_allowed to the
			 * cpuset's cpus_allowed
			 */
			cpumask_copy(new_mask, cpus_allowed);
			goto again;
		}
	}

	free_cpumask_var(new_mask);
out_free_cpus_allowed:
	free_cpumask_var(cpus_allowed);
out_put_task:
	put_task_struct(p);
	put_online_cpus();
	return retval;
}

