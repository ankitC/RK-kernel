#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/current.h>
#include <linux/reserve_framework.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#define S_TO_NS(x)	(x /** 1E9L*/)


enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer )
{
	current->reserve_process->spent_budget.tv_sec = 0;
	current->reserve_process->spent_budget.tv_nsec = 0;

	printk(KERN_INFO "my_hrtimer_callback called (%ld).\n", jiffies );
	return HRTIMER_RESTART;
}

struct hrtimer init_hrtimer( struct timespec T)
{
	ktime_t ktime;
	struct hrtimer hr_timer;
	printk(KERN_INFO "HR Timer installing\n");

	ktime = ktime_set( 0, T.tv_nsec + S_TO_NS(T.tv_sec));

	hrtimer_init( &hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );

	hr_timer.function = &my_hrtimer_callback;

	printk(KERN_INFO "Starting timer to fire in (%ld)\n", jiffies );

	hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_REL );

	return hr_timer;
}

void cleanup_hrtimer(struct hrtimer hr_timer )
{
	int ret;

	ret = hrtimer_cancel( &hr_timer );
	if (ret) printk(KERN_INFO "The timer was still in use...\n");

	printk(KERN_INFO "HR Timer uninstalling\n");

	return;
}

