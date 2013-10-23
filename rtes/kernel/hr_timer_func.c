#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/current.h>
#include <linux/reserve_framework.h>
#include <linux/sysfs_func.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

//struct hrtimer *hr;
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
	/*Asking for reschedule since budget is exhausted*/
	reservation_detail->need_resched = 1;
	set_tsk_need_resched(reservation_detail->monitored_process);
	
		temp = reservation_detail->monitored_process->se.sum_exec_runtime - \
			    reservation_detail->prev_setime;
		reservation_detail->spent_budget = timespec_add\
														(reservation_detail->spent_budget, ns_to_timespec(temp));
		 reservation_detail->monitored_process->reserve_process.prev_setime =  reservation_detail->monitored_process->se.sum_exec_runtime;
	

	spin_unlock_irqrestore(&reservation_detail->reserve_spinlock, flags);

	return HRTIMER_NORESTART;
}
/*
 * Callback function for hr timer
 */
enum hrtimer_restart T_timer_callback( struct hrtimer *T_timer )
{
	struct reserve_obj* reservation_detail=container_of(T_timer,\
			struct reserve_obj, T_timer);
	ktime_t ktime, forward_time, curr_time;
	unsigned long flags;
	spin_lock_irqsave(&reservation_detail->reserve_spinlock, flags);

	printk(KERN_INFO "Budget spent %llu", timespec_to_ns\
			(&reservation_detail->spent_budget));

	ktime = ktime_set(reservation_detail->C.tv_sec, reservation_detail->C.tv_nsec);
	circular_buffer_write(reservation_detail,\
			reservation_detail->spent_budget);
	reservation_detail->spent_budget.tv_sec = 0;
	reservation_detail->spent_budget.tv_nsec = 0;
	reservation_detail->spent_budget.tv_nsec = 0;
	reservation_detail->remaining_C = ktime;

	if (reservation_detail->running)
	{
		hrtimer_cancel(&reservation_detail->C_timer);
		hrtimer_init( &reservation_detail->C_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
		reservation_detail->C_timer.function = &C_timer_callback;
		hrtimer_start(&reservation_detail->C_timer, reservation_detail->remaining_C, HRTIMER_MODE_REL);

	}

	if (reservation_detail->need_resched && reservation_detail->monitored_process->state == TASK_UNINTERRUPTIBLE)
	{
		reservation_detail->need_resched = 0;
		if(!wake_up_process(reservation_detail->monitored_process))
			printk(KERN_INFO "Couldn't wake up process\n");
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
	reservation_detail->monitored_process->under_reservation = 0;

	spin_unlock_irqrestore(&reservation_detail->reserve_spinlock, flags);
	remove_pid_dir_and_reserve_file(reservation_detail->monitored_process);
	return;
}

