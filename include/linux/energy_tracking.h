#ifndef __ENERGY_TRACKING_H_
#define __ENERGY_TRACKING_H_
#include <linux/cpufreq.h>
int get_cpu_energy(unsigned int freq);

unsigned int calculate_sys_clk_freq(int scaling_factor, struct cpufreq_policy * policy);

int cpufreq_set_sysclock(struct cpufreq_policy *policy, unsigned int freq, int flag);
#endif
