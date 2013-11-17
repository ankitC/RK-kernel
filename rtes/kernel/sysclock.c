
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

static DEFINE_PER_CPU(unsigned int, cpu_cur_freq); /* current CPU freq */
/* keep track of frequency transitions */

static int cpufreq_set(struct cpufreq_policy *policy, unsigned int freq)
{
	int ret = -EINVAL;

	printk(KERN_INFO "[%s] Governor Sysclock\n", __func__);

	return ret;
}

static ssize_t show_speed(struct cpufreq_policy *policy, char *buf)
{
	return sprintf(buf, "%u\n", per_cpu(cpu_cur_freq, policy->cpu));
}

static int cpufreq_governor_sysclock(struct cpufreq_policy *policy,
				   unsigned int event)
{

	printk(KERN_INFO "[%s] Governor Sysclock\n", __func__);
	return 0;
}

struct cpufreq_governor cpufreq_gov_sysclock = {
	.name		= "sysclock",
	.governor	= cpufreq_governor_sysclock,
	.store_setspeed	= cpufreq_set,
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
