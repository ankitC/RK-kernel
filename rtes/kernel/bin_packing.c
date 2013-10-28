#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/cpumask.h>
#include <linux/linked_list.h>
#include <linux/cpu_linked_list.h>
#include <linux/reserve_framework.h>
#include <asm/div64.h>
#include <linux/types.h>
#define UNSCHEDULABLE 2

BIN_NODE* cpu_bin_head[4] = {0};
extern const uint32_t bounds_tasks[62];
extern char partition_policy[2]; 
extern BIN_NODE* bin_head;
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

	printk(KERN_INFO "ub test: curr1 util %llu\n", curr1->task->reserve_process.U);

	if (cpu_bin_head[cpu] == NULL && curr1->task->reserve_process.U < bounds_tasks[0])
	{
		add_cpu_node(make_cpu_node(curr1->task), cpu);
		return 1;
	}

	while(curr)
	{

		total_util += curr->task->reserve_process.U;
		printk(KERN_INFO "curr1 util %llu\n", curr1->task->reserve_process.U);
		curr = curr->next;
		i++;
	}

	printk(KERN_INFO "Complete Util: %llu\n", total_util);

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

	printk(KERN_INFO "Value of a[0] = %llu \n", a[0]);

	curr = cpu_bin_head[cpu];

	while (1)
	{
		a[i + 1] = 0;
		while (curr != stop)
		{

			*A_temp = a[i];
			*T_temp = timespec_to_ns(&curr->task->reserve_process.T);
			printk(KERN_INFO "Before: *A_temp = %llu", *A_temp);
			printk(KERN_INFO "Before: *T_temp = %llu", *T_temp);
			remainder = do_div(*T_temp, 10000);
			t = (uint32_t) *T_temp;

			/* Calculating Ceiling */
			remainder = do_div(*A_temp, t);
			if ((remainder = do_div(*A_temp, 10000)) > 0)
			{
				*A_temp = *A_temp + 1;
				printk(KERN_INFO "After: *A_temp = %llu", *A_temp);

			}
			printk(KERN_INFO "After Scaling: *A_temp = %llu", *A_temp);

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
			printk(KERN_INFO "T = %llu", timespec_to_ns(&stop->task->reserve_process.T));
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

	printk(KERN_INFO "RT Test\n");
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
			printk(KERN_INFO "We have another task to run this test on\n");
			curr = curr->next;
		}
		else{
			printk(KERN_INFO "End of Linked List\n");
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
 * First fit heuristic for bin packing
 */
int apply_first_fit(void)
{
	int cpu = 0;
	BIN_NODE* curr = bin_head;

	printk(KERN_INFO "First fit\n");
	while (curr && cpu < 4)
	{
		if (admission_test_for_cpu(curr, cpu) < 0)
		{
			cpu++;
		}
		else
		{
			printk(KERN_INFO "Setting cpu %d", cpu);
			curr->task->reserve_process.host_cpu = cpu;
			curr = curr->next;
			if (curr)
				printk(KERN_INFO "Next node existsi\n");
			cpu = 0;
		}
	}

	if (cpu == 4)
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

	while (curr && counter < 4)
	{
		if (admission_test_for_cpu(curr, cpu) < 0)
		{
			cpu = (cpu + 1) % 4;
			printk(KERN_INFO "Incrementing cpu %d", cpu);
			counter++;
		}
		else
		{
			printk(KERN_INFO "Setting cpu %d", cpu);
			curr->task->reserve_process.host_cpu = cpu;
			curr = curr->next;
		}
	}

	if (counter == 4)
		return -1;
	else
		return 1;
}


/*
 * Best fit heuristic for bin packing
 */
int apply_best_fit(void)
{

	return 0;
}


/*
 * Worst fit heuristic for bin packing
 */
int apply_worst_fit(void)
{

	return 0;
}

/*
 * Applies the heuristic for bin packing in the partition_policy
 * variable. 
 * Returns 1 on success
 * Returns -1 on failure
 */
int apply_heuristic(void)
{
	int retval = 0;

	if (strcmp(partition_policy,"N") == 0)
			retval = apply_next_fit();
	if (strcmp(partition_policy,"B") == 0)
			retval = apply_best_fit();
	if (strcmp(partition_policy,"W") == 0)
			retval = apply_worst_fit();
	if (strcmp(partition_policy,"F") == 0)
			retval = apply_first_fit();

	delete_all_cpu_nodes();

	return retval;
}
