/*
	Sysclock Governor for Kernel.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#define MAX_SCALING_FACTOR 100
/**
 * A few values needed by the sysclock governor
 */

DEFINE_MUTEX(sysclock_mutex);
DEFINE_SPINLOCK(scaling_spinlock);
int cpus_using_sysclock_governor = 0;
extern unsigned int sysclock_scaling_factor;
extern unsigned int global_sysclock_freq;
unsigned int global_scaling_factor = MAX_SCALING_FACTOR;

static ssize_t show_speed(struct cpufreq_policy *policy, char *buf)
{
	printk(KERN_INFO "[%s] \n", __func__);
	return sprintf(buf, "%u\n", per_cpu(cpu_cur_freq, policy->cpu));
}

/**
 * cpufreq_set - set the CPU frequency
 * @policy: pointer to policy struct where freq is being set
 * @freq: target frequency in kHz
 *
 * Sets the CPU frequency to freq.
 */
int cpufreq_set_sysclock(struct cpufreq_policy *policy, unsigned int freq, int heuristic_flag)
{
	int ret = -EINVAL;
	uint32_t remainder = 0;
	uint64_t S_var = 0;
	uint64_t *S_temp = &S_var;
	unsigned long flags = 0;
	
	/* Setting Frequency of system as per sysclock */
	mutex_lock(&sysclock_mutex);
	if (cpus_using_sysclock_governor != 0)
	{
		mutex_unlock(&sysclock_mutex);
		if (freq == 0 || strcmp(policy->governor->name, "sysclock") != 0)
		{
			printk("[%s] returning without setting the sysclock %s\n", __func__,  policy->governor->name);
			return 0;
		}

		printk("[%s] freq %u kHz\n", __func__, freq);

		ret = __cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_L);

		if (ret < 0)
			printk("[%s] Setting freq failed\n", __func__);

		
		spin_lock_irqsave(&scaling_spinlock, flags);
		S_var = policy->max * MAX_SCALING_FACTOR;
		remainder = do_div(*S_temp, freq);
		global_scaling_factor = S_var;
		printk("[%s] global_scaling_factor : %u\n", __func__, global_scaling_factor);
		spin_unlock_irqrestore(&scaling_spinlock, flags);
		return ret;
	}
	mutex_unlock(&sysclock_mutex);
	return ret;
}

/* Handling callbacks from processor for the Sysclock Governor */
static int cpufreq_governor_sysclock(struct cpufreq_policy *policy,unsigned int event)
{
	unsigned int cpu = policy->cpu;
	unsigned int local_sys_freq = 0;
	int rc = 0;

	switch (event) {
		case CPUFREQ_GOV_START:

			if (!cpu_online(cpu))
				return -EINVAL;

			mutex_lock(&sysclock_mutex);
			cpus_using_sysclock_governor++;
			local_sys_freq = global_sysclock_freq;
			mutex_unlock(&sysclock_mutex);

			//Setting sysclock calculated frequency
			if (cpufreq_set_sysclock(policy, local_sys_freq, 0) < 0)
				return -EINVAL;
			printk(KERN_INFO "[%s] START policy->cur %u", __func__, policy->cur);
			break;
		
		case CPUFREQ_GOV_STOP:
			/* Decrease number of CPUs running at sysclock when a CPU shuts down */
			mutex_lock(&sysclock_mutex);
			cpus_using_sysclock_governor--;

			// Make the global sysclock values default when changing the governor
			if (!cpus_using_sysclock_governor)
			{
				global_sysclock_freq = 0;
				global_scaling_factor = MAX_SCALING_FACTOR;
			}
			mutex_unlock(&sysclock_mutex);
			
			break;

		case CPUFREQ_GOV_LIMITS:
		/* Setting the min limit in for the policy */
		__cpufreq_driver_target(policy, policy->min,CPUFREQ_RELATION_L);
			break;
	}
	return rc;
}

struct cpufreq_governor cpufreq_gov_sysclock = {
	.name		= "sysclock",
	.governor	= cpufreq_governor_sysclock,
	.store_setspeed = NULL,
	.show_setspeed	= show_speed,
	.owner		= THIS_MODULE,
};

static int __init cpufreq_gov_sysclock_init(void)
{
	printk(KERN_INFO "[%s] Initialising Sysclock Governor\n", __func__);
	return cpufreq_register_governor(&cpufreq_gov_sysclock);
}


static void __exit cpufreq_gov_sysclock_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_sysclock);
}

MODULE_DESCRIPTION("CPUfreq policy governor 'sysclock'");
MODULE_LICENSE("GPL");

module_init(cpufreq_gov_sysclock_init);
module_exit(cpufreq_gov_sysclock_exit);
