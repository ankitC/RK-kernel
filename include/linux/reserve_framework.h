#ifndef RESERVE_FRAMEWORK_H
#define RESERVE_FRAMEWORK_H

#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/spinlock.h>
#include <linux/kobject.h>
#include <linux/kobject.h>
#include <asm/spinlock.h>
#include <asm/page.h>

//#include <linux/sched.h>
typedef struct circ_buff
{
	//char buffer[PAGE_SIZE];
	char buffer[96];
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
	int signal_sent;
	int buffer_overflow;
	struct timespec C;
	struct timespec T;
	struct timespec spent_budget;
	struct hrtimer hr_timer;

	struct kobj_attribute util_attr;
	struct kobj_attribute overflow_attr;
	struct kobject *pid_obj;

	struct hrtimer C_timer;
	ktime_t remaining_C_time;

	spinlock_t reserve_spinlock;
	struct attribute *attrs[3];
	circular_buffer c_buf;
};


void circular_buffer_write(struct reserve_obj* res_detail, struct timespec spent_budget);
int circular_buffer_read(struct reserve_obj* res_detail , char* buf);
<<<<<<< HEAD
int create_switches(struct kobject *config_obj);
void stop_c_timer(struct task_struct * task);
void start_c_timer(struct task_struct * task);
=======
>>>>>>> parent of bade3aa... Sysfs code for switches
#endif /* RESERVE_FRAMEWORK_H */
