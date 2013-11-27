#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/current.h>
#include <linux/reserve_framework.h>
#include <linux/linked_list.h>
#include <linux/bin_linked_list.h>
#include <linux/cpu_linked_list.h>
#include <linux/sysfs_func.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/energy_tracking.h>
#include <linux/ktime.h>

extern spinlock_t scaling_spinlock;
extern unsigned int global_scaling_factor;
/*
 * Callback function for C timer
 */
enum hrtimer_restart C_timer_callback( struct hrtimer *C_timer )
{
	struct reserve_obj* reservation_detail=container_of(C_timer,\
			struct reserve_obj, C_timer);

	unsigned long long temp;
	unsigned long flags;

	spin_lock_irqsave(&reservation_detail->reserve_spinlock, flags);

	printk(KERN_INFO "PID:%d->C_Timer Expired.\n", reservation_detail->pid);

	/*Asking for reschedule since budget is exhausted*/
	reservation_detail->need_resched = 1;
	set_tsk_need_resched(reservation_detail->monitored_process);
		temp = reservation_detail->monitored_process->se.sum_exec_runtime - \
			    reservation_detail->prev_setime;
		reservation_detail->spent_budget = timespec_add\
														(reservation_detail->spent_budget, ns_to_timespec(temp));
	reservation_detail->monitored_process->reserve_process->prev_setime =  reservation_detail->monitored_process->se.sum_exec_runtime;
//TODO reservation_detail->prev_setime =  reservation_detail->monitored_process->se.sum_exec_runtime;
	spin_unlock_irqrestore(&reservation_detail->reserve_spinlock, flags);

	return HRTIMER_NORESTART;
}

/*
 * Callback function for T timer
 */
enum hrtimer_restart T_timer_callback( struct hrtimer *T_timer )
{
//	printk(KERN_INFO "HR T TIMER CALL BACK");
	struct reserve_obj* reservation_detail=container_of(T_timer,\
			struct reserve_obj, T_timer);
	ktime_t ktime, forward_time, curr_time;

	unsigned long flags;
	unsigned long long temp;
	unsigned long long temp_remaining_C = 0;
	uint64_t C_var = 0;
	uint64_t* C_temp = &C_var;
	uint32_t remainder = 0;


	spin_lock_irqsave(&reservation_detail->reserve_spinlock, flags);

	ktime = ktime_set(reservation_detail->C.tv_sec, reservation_detail->C.tv_nsec);
	reservation_detail->remaining_C = ktime;

	/* Book keeping in case the task is running while its callback happens */
	if (reservation_detail->running)
	{
		temp = reservation_detail->monitored_process->se.sum_exec_runtime - \
			   reservation_detail->prev_setime;
		reservation_detail->spent_budget = timespec_add\
				(reservation_detail->spent_budget, ns_to_timespec(temp));
		reservation_detail->prev_setime = \
				reservation_detail->monitored_process->se.sum_exec_runtime;

		hrtimer_cancel(&reservation_detail->C_timer);
		hrtimer_init( &reservation_detail->C_timer, \
				CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED );
		reservation_detail->C_timer.function = &C_timer_callback;
		temp_remaining_C = ktime_to_ns(reservation_detail->remaining_C);
		//mutex_lock(&scaling_mutex);
		spin_lock_irqsave(&scaling_spinlock, flags);
		reservation_detail->local_scaling_factor = global_scaling_factor;
		C_var = temp_remaining_C * reservation_detail->local_scaling_factor;
		spin_unlock_irqrestore(&scaling_spinlock, flags);
		//mutex_unlock(&scaling_mutex);
		remainder = do_div(*C_temp, 100);
		hrtimer_start(&reservation_detail->C_timer, ktime_set(0, C_var), HRTIMER_MODE_REL_PINNED);


		//hrtimer_start(&reservation_detail->C_timer,reservation_detail->remaining_C, HRTIMER_MODE_REL_PINNED);
	}

	printk(KERN_INFO "PID:%d->Budget spent:%llu", reservation_detail->pid, timespec_to_ns\
			(&reservation_detail->spent_budget));

	circular_buffer_write(reservation_detail,\
			reservation_detail->spent_budget);
	reservation_detail->spent_budget.tv_sec = 0;
	reservation_detail->spent_budget.tv_nsec = 0;

	/* Waking the task up if it was deactivated due to overspent budget */
	if (reservation_detail->need_resched && reservation_detail->monitored_process->state == TASK_UNINTERRUPTIBLE)
	{
		reservation_detail->need_resched = 0;
		if(!wake_up_process(reservation_detail->monitored_process))
			printk(KERN_INFO "Couldn't wake up process\n");
		else
			printk(KERN_INFO "PID:%d->Resuming Task.\n", reservation_detail->pid);
	}

	forward_time = ktime_set(reservation_detail->T.tv_sec\
			, reservation_detail->T.tv_nsec);

	curr_time = ktime_get();
	hrtimer_forward(T_timer, curr_time, forward_time);
	spin_unlock_irqrestore(&reservation_detail->reserve_spinlock, flags);
	return HRTIMER_RESTART;
}


/*
 * Cleans up the hr timer for each reserved task whose reservation is cancelled
 */

void cleanup_hrtimer(struct hrtimer *T_timer )
{
	struct reserve_obj* reservation_detail=container_of(T_timer,\
			struct reserve_obj, T_timer);

	unsigned long flags;
	spin_lock_irqsave(&reservation_detail->reserve_spinlock, flags);
	printk(KERN_INFO "Cancelling reservation %d\n", reservation_detail->pid);

	if (!hrtimer_cancel( T_timer ))
	{
		printk(KERN_INFO "Failed to cancel T_timer\n");
	}

	if (!hrtimer_cancel( &reservation_detail->C_timer ))
	{
		printk(KERN_INFO "Failed to cancel C_timer\n");
	}
	delete_node(reservation_detail->monitored_process);
	delete_bin_node(reservation_detail->monitored_process);

	reservation_detail->monitored_process->under_reservation = 0;

	spin_unlock_irqrestore(&reservation_detail->reserve_spinlock, flags);
	remove_pid_dir_and_reserve_file(reservation_detail->monitored_process);
	return;
}

