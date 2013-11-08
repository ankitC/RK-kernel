#ifndef RESERVE_FRAMEWORK_H
#define RESERVE_FRAMEWORK_H

#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/spinlock.h>
#include <linux/kobject.h>
#include <linux/kobject.h>
#include <linux/sched.h>
#include <asm/spinlock.h>
#include <asm/page.h>

//#include <linux/sched.h>
typedef struct circ_buff
{
	char buffer[PAGE_SIZE];
	//char buffer[96];
	int start;
	int end;
	int read_count;
} circular_buffer;

struct reserve_obj
{
	char name[20];
	pid_t pid;
	unsigned long long prev_setime;
	struct task_struct *monitored_process;
	int need_resched;
	int buffer_overflow;
	int ctx_overflow;
	int t_timer_started;
	int pending;
	int deactivated;
	int running;
	int rt_prio;
	unsigned int host_cpu;
	unsigned int prev_cpu;
	struct timespec C;
	unsigned long long U;
	struct timespec T;
	struct timespec spent_budget;
	struct hrtimer T_timer;
	struct hrtimer C_timer;
	ktime_t remaining_C;
	struct kobj_attribute util_attr;
	struct kobj_attribute overflow_attr;
	struct kobj_attribute tval_attr;
	struct kobj_attribute ctx_attr;
	struct kobject *pid_obj;
	spinlock_t reserve_spinlock;
	spinlock_t bin_spinlock;
	struct attribute *attrs[5];
	circular_buffer c_buf;
	circular_buffer ctx_buf;
};

void ctx_buffer_write(struct reserve_obj* res_detail, struct timespec spent_budget, int ctx_in);
int ctx_buffer_read(struct reserve_obj* res_detail , char* buf);

void circular_buffer_write(struct reserve_obj* res_detail, struct timespec spent_budget);
int circular_buffer_read(struct reserve_obj* res_detail , char* buf);
long do_calc(long first, long second, char operation);
#endif /* RESERVE_FRAMEWORK_H */
