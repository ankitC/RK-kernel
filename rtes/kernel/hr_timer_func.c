#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/current.h>
#include <linux/reserve_framework.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#define S_TO_NS(x)	(x * 1000000000)

struct hrtimer *hr;

static enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer )
{
	pid_t pid;
	struct reserve_obj* reservation_detail=container_of(timer,\
		   	struct reserve_obj, hr_timer);

	pid = reservation_detail->pid;


	printk(KERN_INFO "my_hrtimer_callback pid %d\n", \
			reservation_detail->pid);
	//current->reserve_process->spent_budget.tv_sec = 0;
	//current->reserve_process->spent_budget.tv_nsec = 0;
	printk(KERN_INFO "name: %s pid: %u prevtime %llu \n Runtime=%llu\n"\
			,reservation_detail->name, pid,\
			reservation_detail->prev_setime, \
			( reservation_detail->monitored_process->se.sum_exec_runtime - \
			  reservation_detail->prev_setime));

	reservation_detail->prev_setime = reservation_detail->monitored_process->\
									  se.sum_exec_runtime;

	ktime_t forward_time = ktime_set(reservation_detail->T.tv_sec\
	, reservation_detail->T.tv_nsec);

	ktime_t curr_time = ktime_get();

	hrtimer_forward(timer, curr_time, forward_time);
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
	int ret;

	ret = hrtimer_cancel( hr_timer );
	kfree(hr_timer);
	//if (ret) printk(KERN_INFO "The timer was still in use...\n");

	//printk(KERN_INFO "HR Timer uninstalling\n");

	return;
}

