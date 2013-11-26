
/*
 *  linux/drivers/cpufreq/cpufreq_sysclock.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2002 - 2004 Dominik Brodowski <linux@brodo.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
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
static DEFINE_PER_CPU(unsigned int, cpu_max_freq);
static DEFINE_PER_CPU(unsigned int, cpu_min_freq);
static DEFINE_PER_CPU(unsigned int, cpu_cur_freq); /* current CPU freq */
static DEFINE_PER_CPU(unsigned int, cpu_set_freq); /* CPU freq desired by
													  sysclock */
static DEFINE_PER_CPU(unsigned int, cpu_is_managed);
static DEFINE_PER_CPU(struct cpufreq_policy *, temp_policy);
DEFINE_MUTEX(sysclock_mutex);
DEFINE_SPINLOCK(scaling_spinlock);
static int cpus_using_sysclock_governor;
extern unsigned int sysclock_scaling_factor;
extern unsigned int global_sysclock_freq;
unsigned int global_scaling_factor = MAX_SCALING_FACTOR;
int sysclock_governor_selected = 0;

/* keep track of frequency transitions */
	static int
sysclock_cpufreq_notifier(struct notifier_block *nb, unsigned long val,
		void *data)
{
	struct cpufreq_freqs *freq = data;

	if (!per_cpu(cpu_is_managed, freq->cpu))
		return 0;

	pr_debug("saving cpu_cur_freq of cpu %u to be %u kHz\n",
			freq->cpu, freq->new);
	per_cpu(cpu_cur_freq, freq->cpu) = freq->new;

	return 0;
}

static struct notifier_block sysclock_cpufreq_notifier_block = {
	.notifier_call  = sysclock_cpufreq_notifier
};

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
	printk("[%s] %s\n", __func__,  policy->governor->name);

	if (sysclock_governor_selected)
	{
		if (freq == 0 || strcmp(policy->governor->name, "sysclock") != 0)
		{
			printk("[%s] returning without setting the sysclock %s\n", __func__,  policy->governor->name);
			return 0;
		}

		printk("[%s] for cpu %u, freq %u kHz\n", __func__,  policy->cpu, freq);

		per_cpu(cpu_set_freq, policy->cpu) = freq;

		if (freq < per_cpu(cpu_min_freq, policy->cpu))
			freq = per_cpu(cpu_min_freq, policy->cpu);
		if (freq > per_cpu(cpu_max_freq, policy->cpu))
			freq = per_cpu(cpu_max_freq, policy->cpu);

		ret = __cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_L);

		//mutex_lock(&scaling_mutex);
		spin_lock_irqsave(&scaling_spinlock, flags);
		S_var = policy->max * MAX_SCALING_FACTOR;
		remainder = do_div(*S_temp, freq);
		global_scaling_factor = S_var;
		printk("[%s] global_scaling_factor : %u\n", __func__, global_scaling_factor);
		spin_unlock_irqrestore(&scaling_spinlock, flags);
		//mutex_unlock(&scaling_mutex);
	}
	return ret;
}
static int cpufreq_governor_sysclock(struct cpufreq_policy *policy,
		unsigned int event)
{
	unsigned int cpu = policy->cpu;
	int rc = 0;

	printk(KERN_INFO "[%s] Governor Sysclock\n", __func__);
	printk(KERN_INFO "[%s] policy cpu %d\n", __func__, policy->cpu);
	printk(KERN_INFO "[%s] event %u\n", __func__, event);

	switch (event) {
		case CPUFREQ_GOV_START:
			if (!cpu_online(cpu))
				return -EINVAL;
			per_cpu(temp_policy, cpu) =policy;
			BUG_ON(!policy->cur);
			mutex_lock(&sysclock_mutex);

			if (cpus_using_sysclock_governor == 0) {
				cpufreq_register_notifier(
						&sysclock_cpufreq_notifier_block,
						CPUFREQ_TRANSITION_NOTIFIER);
			}
			cpus_using_sysclock_governor++;

			//Selecting the sysclock governor
			sysclock_governor_selected = 1;
			per_cpu(cpu_is_managed, cpu) = 1;
			per_cpu(cpu_min_freq, cpu) = policy->min;
			per_cpu(cpu_max_freq, cpu) = policy->max;
			per_cpu(cpu_cur_freq, cpu) = policy->cur;
			per_cpu(cpu_set_freq, cpu) = policy->cur;
			pr_debug("managing cpu %u started "
					"(%u - %u kHz, currently %u kHz)\n",
					cpu,
					per_cpu(cpu_min_freq, cpu),
					per_cpu(cpu_max_freq, cpu),
					per_cpu(cpu_cur_freq, cpu));

			mutex_unlock(&sysclock_mutex);

			//Setting sysclock calculated frequency
			if (cpufreq_set_sysclock(policy, global_sysclock_freq, 0) < 0)
				return -EINVAL;


			break;
		case CPUFREQ_GOV_STOP:
			mutex_lock(&sysclock_mutex);
			cpus_using_sysclock_governor--;
			if (cpus_using_sysclock_governor == 0) {
				cpufreq_unregister_notifier(
						&sysclock_cpufreq_notifier_block,
						CPUFREQ_TRANSITION_NOTIFIER);
			}

			per_cpu(cpu_is_managed, cpu) = 0;
			per_cpu(cpu_min_freq, cpu) = 0;
			per_cpu(cpu_max_freq, cpu) = 0;
			per_cpu(cpu_set_freq, cpu) = 0;
			per_cpu(temp_policy, cpu) =NULL;
			pr_debug("managing cpu %u stopped\n", cpu);
			global_sysclock_freq = 0;
			sysclock_governor_selected = 0;
			global_scaling_factor = MAX_SCALING_FACTOR;
			mutex_unlock(&sysclock_mutex);
			break;
		case CPUFREQ_GOV_LIMITS:
			mutex_lock(&sysclock_mutex);
			pr_debug("limit event for cpu %u: %u - %u kHz, "
					"currently %u kHz, last set to %u kHz\n",
					cpu, policy->min, policy->max,
					per_cpu(cpu_cur_freq, cpu),
					per_cpu(cpu_set_freq, cpu));
			if (policy->max < per_cpu(cpu_set_freq, cpu)) {
				__cpufreq_driver_target(policy, policy->max,
						CPUFREQ_RELATION_H);
			} else if (policy->min > per_cpu(cpu_set_freq, cpu)) {
				__cpufreq_driver_target(policy, policy->min,
						CPUFREQ_RELATION_L);
			} else {
				__cpufreq_driver_target(policy,
						per_cpu(cpu_set_freq, cpu),
						CPUFREQ_RELATION_L);
			}
			per_cpu(cpu_min_freq, cpu) = policy->min;
			per_cpu(cpu_max_freq, cpu) = policy->max;
			per_cpu(cpu_cur_freq, cpu) = policy->cur;
			mutex_unlock(&sysclock_mutex);
			//Setting sysclock calculated frequency
			if (cpufreq_set_sysclock(policy, global_sysclock_freq, 0) < 0)
				return -EINVAL;

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
