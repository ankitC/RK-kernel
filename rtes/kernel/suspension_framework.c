#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/hr_timer_func.h>
#include <linux/bin_packing.h>
#include <linux/partition_scheduling.h>
#include <asm/current.h>
#include <linux/delay.h>
//extern spinlock_t(bin_spinlock);
extern BIN_NODE* bin_head;
extern int suspend_processes;
//int wakeup_countdown = 0;

/*
 * Waking up suspended tasks
 */

void wakeup_tasks(void)
{
	ktime_t ktime;
	BIN_NODE *curr = bin_head;

	printk(KERN_INFO "Wkup\n");
	while (curr)
	{
		if (curr->task->state == TASK_UNINTERRUPTIBLE)
		{
			set_cpu_for_task(curr->task);
			ktime = ktime_set(curr->task->reserve_process.C.tv_sec, curr->task->reserve_process.C.tv_nsec);
			curr->task->reserve_process.spent_budget.tv_sec = 0;
			curr->task->reserve_process.spent_budget.tv_nsec = 0;
			curr->task->reserve_process.remaining_C = ktime;
			curr->task->reserve_process.t_timer_started = 0;
			if(!wake_up_process(curr->task))
				printk(KERN_INFO "Couldn't wake up process %d\n", curr->task->pid);

		}
		curr = curr->next;
	}
	suspend_processes = 0;
	printk(KERN_INFO "Wake up task ends\n");
}

/*
 * Suspend tasks from the utilization liked lists
 * When migrate is one and the policy is changed
 */
void migrate_and_start(void)
{
	BIN_NODE* curr = bin_head;
	int flag = 0;

	while (curr)
	{
		if (curr->task->reserve_process.host_cpu != curr->task->reserve_process.prev_cpu)
		{
			curr->task->reserve_process.pending = 1;
			printk(KERN_INFO "In migrate and start %d\n", curr->task->pid);
		}

		curr = curr->next;
	}

	do{
		flag = 0;
		curr = bin_head;
		while (curr)
		{

//			printk(KERN_INFO "Looping %d\n", curr->task->pid);
			//		down(curr->task->reserve_process.sem);
			if (curr->task->reserve_process.pending == 1)
				flag =1;
			curr = curr ->next;
		}
		if(flag == 1)
		//	msleep (200);
		//	set_tsk_need_resched(current);
		yield(); /*Help!*/
	} while(flag >0);

	printk(KERN_INFO "Just before wakeup_tasks\n");
//	wakeup_tasks();
}

void migrate_only(void)
{
	BIN_NODE* curr = bin_head;

	while (curr)
	{
		curr->task->reserve_process.pending = 1;
		curr = curr->next;
	}
	printk(KERN_INFO "Set all to pending migrate only %d\n", curr->task->pid);
}
