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

//#define S_TO_NS(x)	(x * 1000000000)

struct hrtimer *hr;

static enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer )
{
	struct reserve_obj* reservation_detail=container_of(timer,\
			struct reserve_obj, hr_timer);

	ktime_t forward_time, curr_time;
	unsigned long flags;
	spin_lock_irqsave(&reservation_detail->reserve_spinlock, flags);

	printk(KERN_INFO "Lock taken inside callback %d\n", reservation_detail->pid);
	printk(KERN_INFO "name: %s pid: %u prevtime %llu \n Runtime=%llu\n"\
			,reservation_detail->name, reservation_detail->pid,\
			reservation_detail->prev_setime, \
			( reservation_detail->monitored_process->se.sum_exec_runtime - \
			  reservation_detail->prev_setime));

	printk(KERN_INFO "Budget spent %llu", timespec_to_ns\
			(&reservation_detail->spent_budget));

	circular_buffer_write(reservation_detail,\
		   	reservation_detail->spent_budget);
//	reservation_detail->spent_budget = reservation_detail->C;
	reservation_detail->spent_budget.tv_sec = 0;
	reservation_detail->spent_budget.tv_nsec = 0;
	reservation_detail->signal_sent = 0;

	forward_time = ktime_set(reservation_detail->T.tv_sec\
			, reservation_detail->T.tv_nsec);

	curr_time = ktime_get();

	hrtimer_forward(timer, curr_time, forward_time);
	spin_unlock_irqrestore(&reservation_detail->reserve_spinlock, flags);
	printk(KERN_INFO "Lock left inside callback %d\n", reservation_detail->pid);
	return HRTIMER_RESTART;
}

void init_hrtimer( struct reserve_obj * res_p)
{
	ktime_t ktime;
	//printk(KERN_INFO "HR Timer installing\n");
	//	ktime = ktime_set( res_p->T.tv_sec, res_p->T.tv_nsec);
	ktime = ktime_set( 5, 0);

	hrtimer_init( &res_p->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );

	res_p->hr_timer.function = &my_hrtimer_callback;

	//printk(KERN_INFO "Starting timer to fire in (%ld)\n", jiffies );

	hrtimer_start( &res_p->hr_timer, ktime, HRTIMER_MODE_REL );

	//	printk(KERN_INFO "returning from init hr\n");
	return;
}

void cleanup_hrtimer(struct hrtimer *hr_timer )
{
	struct reserve_obj* reservation_detail=container_of(hr_timer,\
			struct reserve_obj, hr_timer);

	unsigned long flags;
	spin_lock_irqsave(&reservation_detail->reserve_spinlock, flags);
	//printk(KERN_INFO "Lock taken inside cleanup %d\n", reservation_detail->pid);
	if (!hrtimer_cancel( hr_timer ))
	{
		printk(KERN_INFO "Failed to cancel hr_timer\n");
	}
	reservation_detail->monitored_process->under_reservation = 0;

	spin_unlock_irqrestore(&reservation_detail->reserve_spinlock, flags);
	remove_pid_dir_and_reserve_file(reservation_detail->monitored_process);
	//printk(KERN_INFO "Lock left inside cleanup %d\n", reservation_detail->pid);
	return;
}

