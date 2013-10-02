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


static enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer )
{
/*	//struct task_struct *task = container_of(container_of(*timer));
//	printk(KERN_INFO "my_hrtimer_callback value computation %lu \n", \
			timespec_to_ns(&task->reserve_process->spent_budget) );
//printk(KERN_INFO "my_hrtimer_callback value computation stime %llu utime %llu\n", \
			task->stime,task->utime);
*/

	printk(KERN_INFO "my_hrtimer_callback pid %d\n", \
			current->pid);
	current->reserve_process->spent_budget.tv_sec = 0;
	current->reserve_process->spent_budget.tv_nsec = 0;

	ktime_t forward_time = ktime_set( 0, timespec_to_ns(&current->reserve_process->T));
	ktime_t curr_time = ktime_get();

	hrtimer_forward(timer, curr_time, forward_time);
		return HRTIMER_RESTART;
}

struct hrtimer* init_hrtimer( struct timespec T)
{
	ktime_t ktime;
	struct hrtimer *hr_timer = kmalloc(sizeof(struct hrtimer), GFP_KERNEL);
	printk(KERN_INFO "HR Timer installing\n");

	ktime = ktime_set( 0, T.tv_nsec + S_TO_NS(T.tv_sec));
	//ktime = ktime_set( 5, 0);

	hrtimer_init( hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );

	(*hr_timer).function = &my_hrtimer_callback;

	printk(KERN_INFO "Starting timer to fire in (%ld)\n", jiffies );

	hrtimer_start( hr_timer, ktime, HRTIMER_MODE_REL );

	printk(KERN_INFO "returning from init hr\n");
	return hr_timer;
}

void cleanup_hrtimer(struct hrtimer *hr_timer )
{
	int ret;

	ret = hrtimer_cancel( hr_timer );
	kfree(hr_timer);
	if (ret) printk(KERN_INFO "The timer was still in use...\n");

	printk(KERN_INFO "HR Timer uninstalling\n");

	return;
}

