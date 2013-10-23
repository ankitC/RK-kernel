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
enum hrtimer_restart C_timer_callback( struct hrtimer *timer )
{
	printk(KERN_INFO "In C timer call back\n");
	struct reserve_obj* reservation_detail=container_of(timer,\
	  struct reserve_obj, C_timer);

	unsigned long flags;
	spin_lock_irqsave(&reservation_detail->reserve_spinlock, flags);
	/*Asking for reschedule since budget is exhausted*/
	reservation_detail->need_resched = 1;
	set_tsk_need_resched(reservation_detail->monitored_process);
	spin_unlock_irqrestore(&reservation_detail->reserve_spinlock, flags);

	return HRTIMER_NORESTART;
}
/*
 * Callback function for hr timer
 */
enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer )
{
	struct reserve_obj* reservation_detail=container_of(timer,\
			struct reserve_obj, hr_timer);
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
		if (!hrtimer_cancel(&reservation_detail->C_timer))
			printk(KERN_INFO "Couldn't cancel hrtimer\n");
		hrtimer_init( &reservation_detail->C_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
		reservation_detail->C_timer.function = &C_timer_callback;
		hrtimer_start(&reservation_detail->C_timer, reservation_detail->remaining_C, HRTIMER_MODE_REL);

	}

	/*if (reservation_detail->need_resched)
	{
		reservation_detail->need_resched = 0;
		if(!wake_up_process(reservation_detail->monitored_process))
			printk(KERN_INFO "Couldn't wake up process\n");
	}*/
	forward_time = ktime_set(reservation_detail->T.tv_sec\
			, reservation_detail->T.tv_nsec);

	curr_time = ktime_get();

	hrtimer_forward(timer, curr_time, forward_time);
	spin_unlock_irqrestore(&reservation_detail->reserve_spinlock, flags);
	return HRTIMER_RESTART;
}

/*
 * Initializes the hr timer for each reserved task
 */
void init_hrtimer( struct reserve_obj * res_p)
{
	ktime_t ktime;
	ktime = ktime_set( res_p->T.tv_sec, res_p->T.tv_nsec);

	if (res_p->running)
	{
		hrtimer_init( &res_p->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
		res_p->hr_timer.function = &my_hrtimer_callback;
		hrtimer_start( &res_p->hr_timer, ktime, HRTIMER_MODE_REL );
	
		printk(KERN_INFO "Started C and T timer\n");
	
		hrtimer_init( &res_p->C_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
		res_p->C_timer.function = &C_timer_callback;
		hrtimer_start( &res_p->C_timer, res_p->remaining_C, HRTIMER_MODE_REL );
		res_p->t_timer_started = 1;
	}

	return;
}


/*
 * Cleans up the hr timer for each reserved task whose reservation is cancelled
 */

void cleanup_hrtimer(struct hrtimer *hr_timer )
{
	struct reserve_obj* reservation_detail=container_of(hr_timer,\
			struct reserve_obj, hr_timer);

	unsigned long flags;
	spin_lock_irqsave(&reservation_detail->reserve_spinlock, flags);
	printk(KERN_INFO "Cancelling reservation %d\n", reservation_detail->pid);
	if (!hrtimer_cancel( hr_timer ))
	{
		printk(KERN_INFO "Failed to cancel hr_timer\n");
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

