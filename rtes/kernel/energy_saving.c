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

void energy_savings(void)
{
	BIN_NODE* curr = bin_head;
	int cpu_used[TOTAL_CORES] = {0, 0, 0, 0};
	int i = 0;

	if (guarantee)
	{
		while (curr)
		{
			cpu_used[curr->task->reserve_process.host_cpu]++;
			curr = curr->next;
		}

		for (i = 0; i < TOTAL_CORES; i++)
		{
			if (cpu_used[i] == 0)
				set_cpu_online(i, 0);
		}
	}
}
