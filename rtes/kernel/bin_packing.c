#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/cpumask.h>
#include <linux/linked_list.h>
#include <linux/cpu_linked_list.h>
#include <linux/reserve_framework.h>
#include <linux/partition_scheduling.h>
#include <linux/custom_heuristic.h>
#include <asm/div64.h>
#include <linux/types.h>
#define UNSCHEDULABLE 2
#define TOTAL_CORES 4
BIN_NODE* cpu_bin_head[TOTAL_CORES] = {0};
extern const uint32_t bounds_tasks[62];
extern char partition_policy[2]; 
extern BIN_NODE* bin_head;
/*
 * Deciding the real time priorities of a task based on period
 */
void set_rt_prios(void)
{
	BIN_NODE *curr = bin_head, *min_node = NULL;
	int rt_prio = MAX_RT_PRIO - 2;
	unsigned long long T_min = ULLONG_MAX, temp = 0;
	int list_len = 0;

	while (curr)
	{
		curr->task->reserve_process.rt_prio = -1;
		curr = curr->next;
		list_len++;
	}

	curr = bin_head;

	while (list_len)
	{
		curr = bin_head;
		T_min = ULLONG_MAX;
		min_node = NULL;

		while (curr)
		{
			if (curr->task->reserve_process.rt_prio == -1)
			{
				temp = timespec_to_ns(&curr->task->reserve_process.T);

				if (T_min > temp)
				{
					min_node = curr;
					T_min = timespec_to_ns(&curr->task->reserve_process.T);
				}
			}
			curr = curr->next;
		}
		if (min_node)
			min_node->task->reserve_process.rt_prio = rt_prio--;
		list_len--;
	}
}


/*
 * Utilization bound test to check for a task
 * Returns UNSCHEDULABLE on Failure.
 * Returns 0 when RT test is required.
 * Returns 1 on Success.
 */
int ub_cpu_test(BIN_NODE *curr1, int cpu)
{

	BIN_NODE *curr = cpu_bin_head[cpu];
	unsigned int i = 0;
	unsigned long long total_util = curr1->task->reserve_process.U;

	if (cpu_bin_head[cpu] == NULL && curr1->task->reserve_process.U < bounds_tasks[0])
	{
		add_cpu_node(make_cpu_node(curr1->task), cpu);
		return 1;
	}

	while(curr)
	{

		total_util += curr->task->reserve_process.U;
		curr = curr->next;
		i++;
	}

	if (total_util	> bounds_tasks[0])
		return UNSCHEDULABLE;
	if(total_util > bounds_tasks[i])
		return 0;

	add_cpu_node(make_cpu_node(curr1->task), cpu);
	return 1;
}
/*
 * Calculation for RT-Test on per task basis.
 * Returns 1 on Success and 0 on failure
 */
int check_cpu_schedulabilty(BIN_NODE *stop, int cpu)
{
	unsigned long long a[24] = {0};
	int i = 0;

	uint64_t A_var = 0;
	uint64_t T_var = 0;
	uint64_t* A_temp = (uint64_t*)&A_var;
	uint64_t* T_temp = (uint64_t*)&T_var;
	uint32_t remainder= 0, t = 0;
	BIN_NODE *curr = cpu_bin_head[cpu];

	while (curr != stop->next)
	{
		a[0] += timespec_to_ns(&curr->task->reserve_process.C);
		curr = curr->next;
	}

	curr = cpu_bin_head[cpu];

	while (1)
	{
		a[i + 1] = 0;
		while (curr != stop)
		{

			*A_temp = a[i];
			*T_temp = timespec_to_ns(&curr->task->reserve_process.T);
			remainder = do_div(*T_temp, 10000);
			t = (uint32_t) *T_temp;

			/* Calculating Ceiling */
			remainder = do_div(*A_temp, t);
			if ((remainder = do_div(*A_temp, 10000)) > 0)
			{
				*A_temp = *A_temp + 1;

			}

			a[i + 1] += *A_temp * timespec_to_ns(&curr->task->reserve_process.C);
			curr = curr->next;
		}

		a[i + 1] += timespec_to_ns(&stop->task->reserve_process.C);

		if ( a[i] == a[i + 1]){
			printk(KERN_INFO "RT Test succeeds a[%d] = %llu", i, a[i]);
			return 1;
		}

		if (a[i + 1] > timespec_to_ns(&stop->task->reserve_process.T))
		{
			printk(KERN_INFO " RT Test failed a[%d] = %llu", i + 1, a[i]);
			return 0;
		}

		i++;
		curr = cpu_bin_head[cpu];
		*A_temp = 0;
		*T_temp = 0;
	}

	return 0;
}

/*
 * Response time test to check whether the task is schedulable on that cpu
 * Returns 1 for success and 0 for failure
 */
int rt_cpu_test(BIN_NODE* foo, int cpu)
{
	unsigned long long total_util = 0;
	int k = 0;
	BIN_NODE *curr = cpu_bin_head[cpu];
	int j = 0;

	add_cpu_node(make_cpu_node(foo->task), cpu);

	while (curr)
	{
		total_util += curr->task->reserve_process.U;
		if (total_util > bounds_tasks[j]){
			break;
		}
		else
			j++; // position in linked list where the RT test has to be started
		curr = curr->next;
	}

	curr = cpu_bin_head[cpu];

	/* Forwarding the linkedlist upto the position*/
	for (k = 0; k < j; k++)
		curr = curr->next;

	while (curr)
	{
		if(check_cpu_schedulabilty(curr, cpu)){
			curr = curr->next;
		}
		else{
			return 0;
		}
	}

	return 1;
}

/*
 * Admission test for a particular cpu
 */
int admission_test_for_cpu(BIN_NODE* curr, int cpu)
{
	int retval = 0;

	retval = ub_cpu_test(curr, cpu);

	if (retval == UNSCHEDULABLE)
		return -1;

	if (retval || rt_cpu_test(curr, cpu))
	{
		return 1;
	}
	else
	{
		delete_cpu_node(curr->task, cpu);
		return -1;
	}
}
/*
 * sort_cpus_util_wf: Sorts cpus in descending order of utilization
 */

void sort_cpus_util_wf(int sorted_cpus[TOTAL_CORES])
{

	BIN_NODE* curr;
	int i = 0;
	int j = 0;


	struct cpu{

		int cpu;
		int util;
	};

	struct cpu cpu_arr[TOTAL_CORES];

	struct cpu temp;

	for (i = 0; i < TOTAL_CORES; i++)
	{
		curr = cpu_bin_head[i];
		cpu_arr[i].util = 0;
		cpu_arr[i].cpu = i;

		while(curr)
		{
			cpu_arr[i].util += curr->task->reserve_process.U;
			curr = curr->next;
		}
	}

	for(i = 0; i < TOTAL_CORES; i++)
	{
		for(j = i; j < TOTAL_CORES; j++)
		{
			if(cpu_arr[i].util > cpu_arr[j].util)
			{
				temp = cpu_arr[i];
				cpu_arr[i] = cpu_arr[j];
				cpu_arr[j] = temp;
			}
		}
	}

	for (i = 0; i < TOTAL_CORES; i++)
	{
		sorted_cpus[i] = cpu_arr[i].cpu;
	}


}

/*
 * sort_cpus_util_bf: Sorts cpus in descending order of utilization
 */

void sort_cpus_util_bf(int sorted_cpus[TOTAL_CORES])
{

	BIN_NODE* curr;
	int i = 0;
	int j = 0;


	struct cpu{

		int cpu;
		int util;
	};

	struct cpu cpu_arr[TOTAL_CORES];

	struct cpu temp;

	for (i = 0; i < TOTAL_CORES; i++)
	{
		curr = cpu_bin_head[i];
		cpu_arr[i].util = 0;
		cpu_arr[i].cpu = i;

		while(curr)
		{
			cpu_arr[i].util += curr->task->reserve_process.U;
			curr = curr->next;
		}
	}

	for(i = 0; i < TOTAL_CORES; i++)
	{
		for(j = i; j < TOTAL_CORES; j++)
		{
			if(cpu_arr[i].util < cpu_arr[j].util)
			{
				temp = cpu_arr[i];
				cpu_arr[i] = cpu_arr[j];
				cpu_arr[j] = temp;
			}
		}
	}

	for (i = 0; i < TOTAL_CORES; i++)
	{
		sorted_cpus[i] = cpu_arr[i].cpu;
	}
}
/*
 * First fit heuristic for bin packing
 */
int apply_first_fit(void)
{
	int cpu = 0;
	BIN_NODE* curr = bin_head;

	printk(KERN_INFO "First fit\n");
	while (curr && cpu < TOTAL_CORES)
	{
		if (admission_test_for_cpu(curr, cpu) < 0)
		{
			cpu++;
		}
		else
		{
			curr->task->reserve_process.prev_cpu = curr->task->reserve_process.host_cpu;
			curr->task->reserve_process.host_cpu = cpu;
			curr = curr->next;
			cpu = 0;
		}
	}

	if (cpu == TOTAL_CORES)
		return -1;
	else
		return 1;
}
/*
 * Next fit heuristic for bin packing
 */
int apply_next_fit(void)
{
	int cpu = 0, counter = 0;
	BIN_NODE* curr = bin_head;
	printk(KERN_INFO "Next fit\n");
	while (curr && counter < TOTAL_CORES)
	{
		if (admission_test_for_cpu(curr, cpu) < 0)
		{
			cpu = (cpu + 1) % TOTAL_CORES;
			counter++;
		}
		else
		{
			curr->task->reserve_process.prev_cpu = curr->task->reserve_process.host_cpu;

			curr->task->reserve_process.host_cpu = cpu;
			curr = curr->next;
		}
	}

	if (counter == TOTAL_CORES)
		return -1;
	else
		return 1;
}


/*
 * Best fit heuristic for bin packing
 */
int apply_best_fit(void)
{
	int cpu = 0;
	BIN_NODE* curr = bin_head;
	int sorted_cpus[TOTAL_CORES] = {0, 1, 2, 3};

	printk(KERN_INFO "Best Fit\n");
	while (curr && cpu < TOTAL_CORES)
	{
		if (admission_test_for_cpu(curr, sorted_cpus[cpu]) < 0)
		{
			cpu++;
		}
		else
		{
			curr->task->reserve_process.prev_cpu = curr->task->reserve_process.host_cpu;
			curr->task->reserve_process.host_cpu = sorted_cpus[cpu];
			curr = curr->next;
			sort_cpus_util_bf(sorted_cpus);
			cpu = 0;
		}
	}

	if (cpu == TOTAL_CORES)
		return -1;
	else
		return 1;
}


/*
 * Worst fit heuristic for bin packing
 */
int apply_worst_fit(void)
{
	int cpu = 0;
	BIN_NODE* curr = bin_head;
	int sorted_cpus[TOTAL_CORES] = {0, 1, 2, 3};
	printk(KERN_INFO "Worst fit\n");
	while (curr && cpu < TOTAL_CORES)
	{
		if (admission_test_for_cpu(curr, sorted_cpus[cpu]) < 0)
		{
			cpu++;
		}
		else
		{
			curr->task->reserve_process.prev_cpu = curr->task->reserve_process.host_cpu;
			curr->task->reserve_process.host_cpu = sorted_cpus[cpu];
			curr = curr->next;
			sort_cpus_util_wf(sorted_cpus);
			cpu = 0;
		}
	}

	if (cpu == TOTAL_CORES)
	{
		return -1;
	}
	else
		return 1;
}

/*
 * Applies the heuristic for bin packing in the partition_policy
 * variable. 
 * Returns 1 on success
 * Returns -1 on failure
 */
int apply_heuristic(char policy[2])
{
	int retval = 0;
	int ret_custom = 0;

	if (strncmp(policy,"N", 1) == 0)
		retval = apply_next_fit();
	if (strncmp(policy,"B", 1) == 0)
		retval = apply_best_fit();
	if (strncmp(policy,"W", 1) == 0)
		retval = apply_worst_fit();
	if (strncmp(policy,"F", 1) == 0)
		retval = apply_first_fit();
	if (strncmp(policy,"P", 1) == 0){
		ret_custom = apply_custom_fit();
		if (ret_custom == -1){
			delete_all_cpu_nodes();
			retval = apply_best_fit();
		}
		else
			retval = ret_custom;
	}

	delete_all_cpu_nodes();

	if (retval == 1)
		set_rt_prios();
	printk(KERN_INFO "Apply Heuristic retval %d\n", retval);

	return retval;
}


