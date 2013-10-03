#ifndef RESERVE_FRAMEWORK_H
#define RESERVE_FRAMEWORK_H

#include <linux/time.h>
#include <linux/hrtimer.h>
//#include <linux/sched.h>

struct reserve_obj
{
	char name[20];
	pid_t pid;
	unsigned long long prev_setime;
	struct task_struct *monitored_process;
	struct timespec C;
	struct timespec T;
	struct timespec spent_budget;
	struct hrtimer hr_timer;
};

#endif /* RESERVE_FRAMEWORK_H */
