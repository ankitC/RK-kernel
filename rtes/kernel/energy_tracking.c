#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <asm/div64.h>

/*
 * Power in mW corresponding frequencies in the cpu_freq 
 * table.
 */
int cpu_power_table[17][2] = {
	{51000, 26}, {102000,  26}, {204000, 28}, {340000, 33}, {475000,  39},\
	{640000, 47}, {760000, 54}, {860000, 61}, {1000000, 72}, {1100000, 79},\
	{1150000, 84}, {1200000, 88},{1300000, 97}, {1400000, 106}, {1500000, 116},\
	{1600000, 127}, {1700000, 137}
};

extern int energy;

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

/*
 *Implements energy energy functionality
 */
void energy_accounting(struct task_struct* prev)
{
//	struct timespec ts;
//	unsigned long long run_time = 0;
//	unsigned int kappa = 4420, beta = 25720000;
	uint64_t *energy_consumed = NULL;

	if (energy)
	{
		if (prev->under_reservation)
		{
			printk(KERN_INFO "[%s] %u cpu freq\n", __func__, cpufreq_get(smp_processor_id()));
			prev->reserve_process.energy_consumed += timespec_to_ns(&prev->reserve_process.spent_budget) * get_cpu_energy(cpufreq_get(smp_processor_id()));
			energy_consumed = &prev->reserve_process.energy_consumed;
			do_div(*energy_consumed, 1000000);
			energy_buffer_write(&prev->reserve_process);
		}

		//		if (next->under_reservation)
		//	energy_buffer_write(&next->reserve_process, ts, 1);
	}
}


