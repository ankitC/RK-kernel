#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <asm/div64.h>
#define MAX_SCALING_FACTOR 100
#define CRITICAL_FREQ 228321
#define CRIT_FREQ_CORE 340000

#define POWER_TABLE_ENTRIES 17
/*
 * Power in mW corresponding frequencies in the cpu_freq 
 * table.
 */
int cpu_power_table[POWER_TABLE_ENTRIES][2] = {
	{51000, 28}, {102000,  35}, {204000, 57}, {340000, 100}, {475000,  156},\
	{640000, 240}, {760000, 311}, {860000, 377}, {1000000, 478}, {1100000, 556},\
	{1150000, 596}, {1200000, 638},{1300000, 726}, {1400000, 819}, {1500000, 915},\
	{1600000, 1017}, {1700000, 1122}
};


int get_cpu_energy(unsigned int freq)
{
	int i = 0;

	for (i = 0; i < POWER_TABLE_ENTRIES; i++)
	{
		if (cpu_power_table[i][0] == freq)
			return cpu_power_table[i][1];
	}

	printk(KERN_INFO "[%s] Cpu freq not found\n", __func__);
	return 0;
}
/*
 *	Calculate the sysclock frequency according to the scaling factor
 */
unsigned int calculate_sys_clk_freq(int scaling_factor, struct cpufreq_policy *policy)
{
	int remainder = 0, i = 0;
	uint32_t new_freq = 0;
	int curr_freq = 0;
	unsigned long long F_var = 0;
	uint64_t *F_temp = &F_var;
	uint32_t max_freq = policy->max, min_freq = policy->min;

	printk(KERN_INFO "[%s] scaling_factor = %d", __func__, scaling_factor);
	F_var = (uint64_t) max_freq * scaling_factor;
	remainder = do_div(*F_temp, MAX_SCALING_FACTOR);
	curr_freq = *F_temp;
	printk(KERN_INFO "[%s] frequency calculated %llu found\n", __func__, *F_temp);

	for (i = 0; i < POWER_TABLE_ENTRIES; i++)
	{
		if (curr_freq <= cpu_power_table[i][0])
		{
			new_freq = cpu_power_table[i][0];
			break;
		}
	}

	if (new_freq < CRITICAL_FREQ)
	{
		new_freq = CRIT_FREQ_CORE;
	}

	if (new_freq < min_freq)
		new_freq = min_freq;

	printk(KERN_INFO "[%s] Cpu freq %u found\n", __func__, new_freq);
	return new_freq;
}
