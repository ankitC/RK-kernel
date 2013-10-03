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
	struct reserve_obj* parent_object=container_of(timer,\
		   	struct reserve_obj, hr_timer);
//	parent_object = parent_object - off;

	pid = parent_object->pid;

	printk(KERN_INFO "name: %s cputimeS: %llu cputimeU:%llu pid: %u prevSettime:%lld \n",parent_object->name
			,parent_object->prev_stime,parent_object->prev_utime,pid,\
			parent_object->prev_setime);
/*
	printk(KERN_INFO "CurS:%lld\nPrevS:%lld\n",current->se.sum_exec_runtime,\
   	current->reserve_process->prev_setime);
	//printk(KERN_INFO "my_hrtimer_callback pid %d\n", \
			current->pid);
	//current->reserve_process->spent_budget.tv_sec = 0;
	//current->reserve_process->spent_budget.tv_nsec = 0;

	current->reserve_process->prev_setime = current->se.sum_exec_runtime;

	ktime_t forward_time = ktime_set(current->reserve_process->T.tv_sec\
	, current->reserve_process->T.tv_nsec);
*/
	ktime_t forward_time = ktime_set(5,0);
	ktime_t curr_time = ktime_get();

	hrtimer_forward(timer, curr_time, forward_time);
		return HRTIMER_RESTART;
}

void init_hrtimer( struct reserve_obj * res_p)
{
	ktime_t ktime;
	//printk(KERN_INFO "HR Timer installing\n");
	ktime = ktime_set( res_p->T.tv_sec, res_p->T.tv_nsec);
	//ktime = ktime_set( 5, 0);

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

