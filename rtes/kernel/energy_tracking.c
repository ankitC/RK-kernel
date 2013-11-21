#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <asm/div64.h>

#define CRITICAL_FREQ 228321
/*
 * Power in mW corresponding frequencies in the cpu_freq 
 * table.
 */
int cpu_power_table[17][2] = {
	{51000, 28}, {102000,  35}, {204000, 57}, {340000, 100}, {475000,  156},\
	{640000, 240}, {760000, 311}, {860000, 377}, {1000000, 478}, {1100000, 556},\
	{1150000, 596}, {1200000, 638},{1300000, 726}, {1400000, 819}, {1500000, 915},\
	{1600000, 1017}, {1700000, 1122}
};


int get_cpu_energy(unsigned int freq)
{
	int i = 0;

	for (i = 0; i < 17; i++)
	{
		if (cpu_power_table[i][0] == freq)
			return cpu_power_table[i][1];
	}

	printk(KERN_INFO "[%s] Cpu freq not found\n", __func__);
	return 0;
}
//TODO Check for extreme cases. Incomplete code.
unsigned int calculate_sys_clk_freq(int scaling_factor, unsigned int max_freq)
{
	int remainder = 0, i = 0;
	uint32_t new_freq = 0, prev_freq = 0;
	unsigned long long F_var = 0;
	uint64_t *F_temp = &F_var;

	F_var = (uint64_t) max_freq * scaling_factor;
	remainder = do_div(*F_temp , 100);

	for (i = 0, i < 17; i++)
	{
		if (*F_temp < cpu_power_table[i][1])
		{
			new_freq = cpu_power_table[i][1];
			break;
		}
	}

	if (*F_temp < CRITICAL_FREQ)
	{
		new_freq = 340000;
	}

	return new_freq;
}
