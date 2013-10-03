#ifndef RESERVE_FRAMEWORK_H
#define RESERVE_FRAMEWORK_H

#include <linux/time.h>
#include <linux/hrtimer.h>
//#include <linux/sched.h>

struct reserve_obj
{
	cputime_t prev_stime;
	cputime_t prev_utime;
	pid_t pid;
	struct timespec C;
	struct timespec T;
	struct timespec spent_budget;
	struct hrtimer hr_timer;
	unsigned long long prev_setime;
};

#endif /* RESERVE_FRAMEWORK_H */
