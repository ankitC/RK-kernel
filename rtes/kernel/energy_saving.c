#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/cpumask.h>
#include <linux/linked_list.h>
#include <linux/cpu_linked_list.h>
#include <linux/bin_linked_list.h>
#include <linux/reserve_framework.h>
#include <linux/types.h>
#define TOTAL_CORES 4
extern int guarantee;
extern BIN_NODE* bin_head;
extern spinlock_t bin_spinlock;

/* Disabling unused CPUs for energy saving */
void energy_savings(void)
{

	BIN_NODE* curr = bin_head;
	int cpu_used[TOTAL_CORES] = {0, 0, 0, 0};
	int i = 0;
	unsigned long flags = 0;

	if (guarantee)
	{
		spin_lock_irqsave(&bin_spinlock, flags);
		while (curr)
		{
			cpu_used[curr->task->reserve_process->host_cpu]++;
			curr = curr->next;
		}
		spin_unlock_irqrestore(&bin_spinlock, flags);

		for (i = 1; i < TOTAL_CORES; i++)
		{
			if (cpu_used[i] == 0)
				cpu_down(i);
		}
	}
}
