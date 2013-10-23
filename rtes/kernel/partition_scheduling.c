#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/reserve_framework.h>
void set_cpu_for_task(struct task_struct *task, unsigned int host_cpu)
{
	struct cpumask af_mask;
	cpumask_clear(&af_mask);
	cpumask_set_cpu(host_cpu, &af_mask);
	cpu_up(host_cpu);
	if (sched_setaffinity(task->pid, &af_mask))
		printk(KERN_INFO "Couldn't set task affinity\n");
	else
		printk(KERN_INFO "Affinity set\n");
	task->reserve_process.host_cpu = host_cpu;
}


