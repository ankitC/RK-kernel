#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/spinlock_types.h>
#include <linux/cpumask.h>
#include <linux/linked_list.h>
#include <linux/bin_packing.h>
#include <linux/bin_linked_list.h>
#include <linux/reserve_framework.h>
#include <linux/suspension_framework.h>
#include <asm/div64.h>
#include <asm/current.h>
#include <linux/types.h>

#define SYSCLOCK_SCALING_FACTOR 10000
#define DEFAULT_FREQ_SCALING_FACTOR 100

extern BIN_NODE* cpu_bin_head[4];

unsigned long long min_val (unsigned long long a, unsigned long long b){

	if (a > b)
		return b;
	else
		return a;
}
unsigned long long max_val (unsigned long long a, unsigned long long b){

	if (a < b)
		return b;
	else
		return a;
}

/* @params:
 * node - task under reservation 
 * TODO: Describe the variables.
 */
unsigned long long energy_min_freq (BIN_NODE *node, int cpu_no)
{
	unsigned long long C_i = timespec_to_ns(&node->task->reserve_process->C);
    unsigned long long T_i = timespec_to_ns(&node->task->reserve_process->T);
	unsigned long long D_i = T_i;
	unsigned long long S = 0, I = 0, t = 0, beta = 0, temp_I = 0;
	uint64_t omega_var = 0, B_var = 0, T_var = 0;
	uint64_t* B_temp = (uint64_t*)&B_var;
	uint64_t* T_temp = (uint64_t*)&T_var;
	uint64_t *omega_temp = &omega_var;
	uint32_t remainder= 0, temp = 0;
	int IN_BZP = 1; // Flag to indicate Busy Period
	unsigned long long alpha = 100, delta = 0, omega_p = 0, omega = C_i, beta_sum = 0;
	BIN_NODE *head = cpu_bin_head[cpu_no], *curr = NULL;

	printk(KERN_INFO "T of node = %llu", T_i);
	while (omega < D_i)
	{
		/* Calculations within the busy period */
		if (IN_BZP)
		{
			delta = D_i - omega;
			printk(KERN_INFO "[%s] delta %llu\n", __func__, delta);
			while ((omega < D_i) && (delta > 0))
			{
				for (curr = head; curr != node->next; curr = curr->next)
				{
					T_var = timespec_to_ns(&curr->task->reserve_process->T);
					printk(KERN_INFO "T_var = %llu", T_var);
					remainder = do_div(*T_temp, SYSCLOCK_SCALING_FACTOR);
					temp = *T_temp;
					omega_var = omega;
					remainder = do_div(*omega_temp ,temp);
					remainder = do_div(*omega_temp, SYSCLOCK_SCALING_FACTOR);
					beta_sum += (timespec_to_ns(&curr->task->reserve_process->C) * (omega_var + 1));
					printk(KERN_INFO "[%s] omega in 1 %llu\n", __func__, omega);
				}
				printk(KERN_INFO "[%s] beta_sum %llu\n", __func__, beta_sum);
				omega_p = beta_sum + S;
				beta_sum = 0;
				delta = omega_p - omega;
				omega = omega_p;
			}
			printk(KERN_INFO "[%s] delta %llu\n", __func__, delta);
			printk(KERN_INFO "[%s] omega 1 %llu\n", __func__, omega);
			IN_BZP = 0;
		}
		else	/* Considering the Idle Time till the start of next busy time. */
		{
			for (curr = head; curr != node->next; curr = curr->next)
			{
				T_var = timespec_to_ns(&curr->task->reserve_process->T);

				remainder = do_div(*T_temp, SYSCLOCK_SCALING_FACTOR);
				temp = *T_temp;
				omega_var = omega;
				remainder = do_div(*omega_temp ,temp);
				if((remainder = do_div(*omega_temp, SYSCLOCK_SCALING_FACTOR)) > 0)
					*omega_temp = *omega_temp + 1;
				printk(KERN_INFO "Tj * omega/Tj = %llu", (timespec_to_ns(&curr->task->reserve_process->T) * omega_var));
				temp_I = min_val (((timespec_to_ns(&curr->task->reserve_process->T) * omega_var) - omega), (D_i - omega));

				if (I != 0)
				{
					I = min_val(I, temp_I);
				}
				else
					I = temp_I;
				printk(KERN_INFO "[%s] omega in 2 %llu\n", __func__, omega);
			}

			printk(KERN_INFO "[%s] Idle time %llu\n", __func__, I);
			printk(KERN_INFO "[%s] omega 2 %llu\n", __func__, omega);
			S += I;
			omega += I;
			t = omega;
			beta = omega - S;

			printk(KERN_INFO "[%s] Outside if loop Beta %llu\n", __func__, beta);
			if ((100 * beta) < (t * alpha))
			{
				printk(KERN_INFO "[%s] Inside if loop Beta %llu\n", __func__, beta);

				T_var = t;
				remainder = do_div(*T_temp, SYSCLOCK_SCALING_FACTOR);
				temp = *T_temp;

				B_var = beta * 100;
				remainder = do_div(*B_temp, temp);
				remainder = do_div(*B_temp, SYSCLOCK_SCALING_FACTOR);

				alpha = *B_temp;
				printk(KERN_INFO "[%s] Inside if loop Alpha %llu\n", __func__, alpha);
			}
			IN_BZP = 1;
		}
	}
	printk(KERN_INFO "[%s] Sysclock Alpha %llu\n", __func__, alpha);
	
	/* Returning the minimum sysclock freq for the task */
	return alpha;
}


unsigned long long sysclock_calculation(int cpu_no)
{
	BIN_NODE* curr = cpu_bin_head[cpu_no];
	unsigned long long cpu_sysclock_freq = 0;

	if (curr == NULL)
		return 0;

	printk(KERN_INFO "[%s] Sysclock Calculation\n", __func__);

	/* Finding the energy minimum frequency for each task */
	while (curr != NULL)
	{
		curr->task->reserve_process->sysclk_freq = energy_min_freq(curr, cpu_no);
		curr = curr->next;
	}

	/* Select the max frequency of all the tasks on a CPU */
	curr = cpu_bin_head[cpu_no];
	while (curr != NULL)
	{
		if (cpu_sysclock_freq < curr->task->reserve_process->sysclk_freq)
			cpu_sysclock_freq = curr->task->reserve_process->sysclk_freq;
		curr = curr->next;
	}

	printk(KERN_INFO "[%s] Sysclock Calculation %llu sysclk freq\n", __func__, cpu_sysclock_freq);
	return cpu_sysclock_freq;

}
