#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/hr_timer_func.h>
#include <linux/bin_packing.h>
#include <linux/partition_scheduling.h>
#include <asm/current.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/completion.h>

DECLARE_COMPLETION (wakeup_comp);
DEFINE_SEMAPHORE(wakeup_sem);
DEFINE_MUTEX (suspend_mutex);

//extern spinlock_t(bin_spinlock);
extern BIN_NODE* bin_head;
extern int suspend_processes;
extern int suspend_all;
extern spinlock_t bin_spinlock;
/*
 * Waking up suspended tasks
 */

void wakeup_tasks(void)
{
	ktime_t ktime;
	BIN_NODE *curr = bin_head;

	unsigned long flags = 0;
	printk(KERN_INFO "Wkup\n");

	while (curr)
	{
		if (curr->task->reserve_process.deactivated == 1)
			set_cpu_for_task(curr->task);
		curr = curr->next;
	}

	spin_lock_irqsave(&bin_spinlock, flags);
	curr = bin_head;
	while (curr)
	{
		if (curr->task->reserve_process.deactivated == 1)
		{

			printk(KERN_INFO "Waking up %d\n", curr->task->pid);
			ktime = ktime_set(curr->task->reserve_process.C.tv_sec, curr->task->reserve_process.C.tv_nsec);
			curr->task->reserve_process.spent_budget.tv_sec = 0;
			curr->task->reserve_process.spent_budget.tv_nsec = 0;
			curr->task->reserve_process.remaining_C = ktime;
			curr->task->reserve_process.t_timer_started = 0;
			curr->task->reserve_process.pending = 0;
			curr->task->reserve_process.need_resched = 0;
			if(!wake_up_process(curr->task))
				printk(KERN_INFO "Couldn't wake up process %d\n", curr->task->pid);
			curr->task->reserve_process.deactivated = 0;
		}
		curr = curr->next;
	}
	spin_unlock_irqrestore(&bin_spinlock, flags);
	printk(KERN_INFO "Wake up task ends\n");
}
/*
 * Suspend tasks from the utilization liked lists
 * When migrate is one and the policy is changed
 */
void migrate_and_start(void)
{
	BIN_NODE* curr = bin_head;
	unsigned long flags = 0;
	unsigned int bypass = 1;

	spin_lock_irqsave(&bin_spinlock, flags);
	while (curr)
	{
		if (curr->task->reserve_process.host_cpu != curr->task->reserve_process.prev_cpu)
		{
			curr->task->reserve_process.pending = 1;
			bypass = 0;
			printk(KERN_INFO "In migrate and start %d\n", curr->task->pid);
		}

		curr = curr->next;
	}
	spin_unlock_irqrestore(&bin_spinlock, flags);

	if(!bypass)
	{
		mutex_lock(&suspend_mutex);
		suspend_processes = 1;
		mutex_unlock(&suspend_mutex);
		printk(KERN_INFO "Just before down - semaphore %u\n", wakeup_sem.count);
		//down(&wakeup_sem);
		wait_for_completion(&wakeup_comp);
		printk(KERN_INFO "Just after down - semaphore %u\n", wakeup_sem.count);

	wakeup_tasks();
	}
}

void migrate_only(void)
{
	BIN_NODE* curr = bin_head;
	unsigned long flags = 0;

	spin_lock_irqsave(&bin_spinlock, flags);
	while (curr)
	{
		curr->task->reserve_process.pending = 1;
		curr = curr->next;
	}
	spin_unlock_irqrestore(&bin_spinlock, flags);

	mutex_lock(&suspend_mutex);
	suspend_all = 1;
	mutex_unlock(&suspend_mutex);

	printk(KERN_INFO "Set all to pending migrate\n");
}
