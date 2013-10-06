#ifndef RESERVE_FRAMEWORK_H
#define RESERVE_FRAMEWORK_H

#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/spinlock.h>
#include <asm/spinlock.h>

//#include <linux/sched.h>

struct reserve_obj
{
	char name[20];
	pid_t pid;
	unsigned long long prev_setime;
	struct task_struct *monitored_process;
	int signal_sent;
	struct timespec C;
	struct timespec T;
	struct timespec spent_budget;
	struct hrtimer hr_timer;
	spinlock_t reserve_spinlock;
};

#endif /* RESERVE_FRAMEWORK_H */
