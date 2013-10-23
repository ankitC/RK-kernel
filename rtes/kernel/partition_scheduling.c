#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/reserve_framework.h>
void set_cpu_for_task(struct task_struct *task)
{
	cpumask_clear(&task->reserve_process.mask);
	cpumask_set_cpu(0, &task->reserve_process.mask);
	if (sched_setaffinity(task->pid, &task->reserve_process.mask))
		printk(KERN_INFO "Couldn't set task affinity\n");
}


