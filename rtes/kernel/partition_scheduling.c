#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/cpumask.h>
#include <linux/linked_list.h>
#include <linux/reserve_framework.h>
#include <asm/div64.h>
#include <linux/types.h>
#define CEILING(X) (X-(int)(X) > 0 ? (int)(X+1) : (int)(X))
#define UNSCHEDULABLE 2
extern int guarantee;
PROC_NODE *head = NULL;

/*
 * Liu Leyland bounds for upto 62 tasks
 */
const uint32_t bounds_tasks[62]=
{
	10000,    8284,	7797,	7568,   7434,   7347,   7286,
	7240,	7205,	7177,	7154,	7135,	7119,	7105,	7094,
	7083,	7074,	7066,	7059,	7052,	7047,	7041,	7036,
	7032,	7028,	7024,	7021,	7017,	7014,	7012,	7009,
	7007,	7004,	7002,	7000,	6998,	6996,	6995,	6993,
	6991,	6990,	6988,	6987,	6986,	6985,	6983,	6982,
	6981,	6980,	6979,	6978,	6977,	6976,	6976,	6975,
	6974,	6973,	6973,	6972,	6971,	6971,	6970
};
/*
 * Utilization bound test to check for a task
 */
int ub_test(struct task_struct *task)
{

	PROC_NODE *curr = head;
	unsigned char i = 0;
	unsigned long long sum = 0;
	uint64_t C_var = 0;
	uint64_t T_var = 0;
	uint64_t* C_temp = (uint64_t*)&C_var;
	uint64_t* T_temp = (uint64_t*)&T_var;
	uint32_t remainder= 0, t = 0;

	printk(KERN_INFO "UB Test\n");

	*C_temp = (timespec_to_ns(&task->reserve_process.C));
	*T_temp = timespec_to_ns(&task->reserve_process.T);
	printk(KERN_INFO " before T: %llu\n", *T_temp);
	remainder = do_div(*T_temp, 10000);
	printk(KERN_INFO "after T: %llu\n", *T_temp);
	t = *T_temp;
	remainder = do_div(*C_temp, t);
	sum = sum + *C_temp;

	printk(KERN_INFO "Utilisation for a current task: %llu\n", *C_temp);

	if (head == NULL && sum < bounds_tasks[0])
	{
		printk(KERN_INFO "T: %llu\n", *T_temp);
	printk(KERN_INFO "Utilisation: %llu\n", sum);


		return 1;
	}

	while(curr)
	{
		*C_temp =(uint64_t) (timespec_to_ns(&curr->task->reserve_process.C));
		*T_temp = timespec_to_ns(&curr->task->reserve_process.T);
		remainder = do_div(*T_temp, 10000);
		t = *T_temp;
		remainder = do_div(*C_temp, t);
		sum = sum + *C_temp;
		printk(KERN_INFO "Utilisation for a prev task: %llu\n", *C_temp);
//		sum = sum + do_calc(C_temp, T_temp, '/') ;

		curr = curr->next;
		i++;
	}


	printk(KERN_INFO "Complete Util: %llu\n", sum);
//	sum = sum + do_calc(C_temp, T_temp, '/') ;

	if (sum > bounds_tasks[0])
		return UNSCHEDULABLE;
	if(sum > bounds_tasks[i])
		return 0;

	return 1;
}
/*
 * Response time test to check whether the task is schedulable on that cpu
 */
int rt_test(struct task_struct *task)
{
	unsigned long long *a;
	unsigned long long C_task = timespec_to_ns(&task->reserve_process.C), temp = 0;
	PROC_NODE * curr = head, *curr1 = head;
	int len = 0, i = 0;

	printk(KERN_INFO "RT Test\n");
	while(curr1)
	{
		curr1 = curr1->next;
		len++;
	}

	a = kzalloc(sizeof(unsigned long long) * len, GFP_KERNEL);

	while (curr)
	{
		a[0] += timespec_to_ns(&curr->task->reserve_process.C);
		curr = curr->next;
	}
	a[0] += timespec_to_ns(&task->reserve_process.C);

	curr = head;

	while (1)
	{
		while (curr)
		{
	//		temp += (CEILING(do_calc(a[i],timespec_to_ns\
							(&curr->task->reserve_process.T), '/')))\
					*timespec_to_ns(&curr->task->reserve_process.C);
			curr = curr->next;
		}

		a[i + 1] = C_task + temp;

		if ( a[i] == a[i + 1])
			return 1;
		if (a[i + 1] > timespec_to_ns(&task->reserve_process.T))
			return 0;
		i++;
		curr = head;
		temp = 0;
	}
	kfree(a);
	return 0;
}

/*
 * Sets a the specified "host_cpu" for the task
 */
void set_cpu_for_task(struct task_struct *task, unsigned int host_cpu)
{
	struct cpumask af_mask;
	cpumask_clear(&af_mask);
	cpumask_set_cpu(host_cpu, &af_mask);
	cpu_up(host_cpu);

	if (sched_setaffinity(task->pid, &af_mask))
		printk(KERN_INFO "Couldn't set task affinity\n");
	else
		printk(KERN_INFO "Affinity set\n");

	task->reserve_process.host_cpu = host_cpu;
}

/*
 * Admission Test for allowing tasks to be dispatched onto a certai processor
 * Admission test is contingent on the guarntee witch in sysfs
 */
int admission_test(struct task_struct *task)
{
	int retval = 0, online_nodes = 0, i = 0;
	if (!guarantee)
		return 0;

	for( i = 0; i < 4; i++)
	{
		if(cpu_online(i))
		{
			online_nodes++;
		}
	}

	/*
	 * Admission test for one cpu
	 */
	if ((online_nodes == 1))
	{
		retval = ub_test(task);

		if (retval == UNSCHEDULABLE)
			return -1;

		if (retval /*|| rt_test(task)*/)
		{
			printk(KERN_INFO "Adding node\n");
			add_ll_node(make_node(task));
			return 0;
		}
	}
	/*
	 * Admission test for many processors
	 */

/*	else
	{
		suspend_all_tasks();
		partition_tasks();
	}*/
	return -1;
}
