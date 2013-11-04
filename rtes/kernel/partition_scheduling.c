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
#include <asm/div64.h>
#include <asm/current.h>
#include <linux/types.h>
#define UNSCHEDULABLE 2
extern int guarantee;
extern int migrate;
PROC_NODE *head = NULL;
extern BIN_NODE *bin_head;
//spinlock_t bin_lock;
DEFINE_SPINLOCK(bin_spinlock);
DEFINE_SPINLOCK(suspend_spinlock);
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
 * Returns UNSCHEDULABLE on Failure.
 * Returns 0 when RT test is required.
 * Returns 1 on Success.
 */
int ub_test(struct task_struct *task)
{

	PROC_NODE *curr = head;
	unsigned int i = 0;
	unsigned long long total_util = task->reserve_process.U;

	if (head == NULL && task->reserve_process.U < bounds_tasks[0])
	{
		add_ll_node(make_node(task));
		return 1;
	}

	while(curr)
	{
		total_util += curr->task->reserve_process.U;
		curr = curr->next;
		i++;
	}

	printk(KERN_INFO "Complete Util: %llu\n", total_util);

	if (total_util	> bounds_tasks[0])
		return UNSCHEDULABLE;
	if(total_util > bounds_tasks[i])
		return 0;

	add_ll_node(make_node(task));
	return 1;
}

/*
 * Calculation for RT-Test on per task basis.
 * Returns 1 on Success and 0 on failure
 */
int check_schedulabilty(PROC_NODE *stop)
{
	unsigned long long a[24] = {0};
	int i = 0;

	uint64_t A_var = 0;
	uint64_t T_var = 0;
	uint64_t* A_temp = (uint64_t*)&A_var;
	uint64_t* T_temp = (uint64_t*)&T_var;
	uint32_t remainder= 0, t = 0;
	PROC_NODE *curr = head;

	while (curr != stop->next)
	{
		a[0] += timespec_to_ns(&curr->task->reserve_process.C);
		curr = curr->next;
	}

	printk(KERN_INFO "Value of a[0] = %llu \n", a[0]);

	curr = head;

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
			printk(KERN_INFO "a[%d] = %llu", i, a[i]);
			return 1;
		}

		if (a[i + 1] > timespec_to_ns(&stop->task->reserve_process.T))
		{
			printk(KERN_INFO "a[%d] = %llu", i + 1, a[i]);
			printk(KERN_INFO "T = %llu", timespec_to_ns(&stop->task->reserve_process.T));
			return 0;
		}

		i++;
		curr = head;
		*A_temp = 0;
		*T_temp = 0;
	}

	return 0;
}

/*
 * Response time test to check whether the task is schedulable on that cpu
 * Returns 1 for success and 0 for failure
 */
int rt_test(struct task_struct *task)
{
	unsigned long long total_util = 0;
	int k = 0;
	PROC_NODE * curr = head;
	int j = 0;

	printk(KERN_INFO "RT Test\n");
	add_ll_node(make_node(task));

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

	curr = head;

	/* Forwarding the linkedlist upto the position*/
	for (k = 0; k < j; k++)
		curr = curr->next;

	while (curr)
	{
		if(check_schedulabilty(curr)){
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


extern char partition_policy[2];
/*
 * Admission Test for allowing tasks to be dispatched onto a certai processor
 * Admission test is contingent on the guarntee switch in sysfs
 * Returns -1 on failure and node is not added to the list.
 * Returns  1 on Success and node is added to the list.
 */

int admission_test(struct task_struct *task)
{
	int retval = 0, online_nodes = 0, i = 0;
	//BIN_NODE* curr = bin_head;
	unsigned long flags;

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

		if (retval || rt_test(task))
		{
			return 1;
		}
		else
		{
			delete_node(task);
			return -1;
		}

	}
	/*
	 * Admission test for many processors
	 */
	else
	{

		spin_lock_irqsave(&bin_spinlock, flags);
		add_bin_node(make_bin_node(task));

		retval = apply_heuristic(partition_policy);

		if ((retval == 1) && (migrate == 1))
		{

			spin_unlock_irqrestore(&bin_spinlock, flags);
			printk(KERN_INFO "Reutning 1 in adm test\n");
			return 1;
		}
		else if ((retval == 1) && (migrate == 0))
		{
			/* Mark all tasks as pending*/
			
			current->reserve_process.need_resched = 1;
			current->reserve_process.suspension_required = 1;
			spin_unlock_irqrestore(&bin_spinlock, flags);
			set_tsk_need_resched(current);
			
			/*spin_lock_irqsave(&suspend_spinlock, flags);
			suspend = 1;
			spin_unlock_irqrestore(&suspend_spinlock, flags);

			set_tsk_need_resched(curr->task);
			spin_unlock_irqrestore(&bin_spinlock, flags);*/
			return 1;
		}

		if (retval < 0)
			printk("Task cannot be scheduled according to heuristic\n");

		spin_unlock_irqrestore(&bin_spinlock, flags);
	}
	return -1;
}

/*
 * Sets a the specified "host_cpu" for the task
 */
void set_cpu_for_task(struct task_struct *task)
{
	struct cpumask af_mask;
	int host_cpu  = task->reserve_process.host_cpu;
if(task!= NULL){
	pid_t pid = task->pid;
	printk(KERN_INFO "Before checking task->under_reservation\n");

	if (task->under_reservation)
	{
		cpu_up(host_cpu);
		cpumask_clear(&af_mask);
		cpumask_set_cpu(host_cpu, &af_mask);
		printk(KERN_INFO "Just before sched_setaffinity\n");
			if (sched_setaffinity(pid, &af_mask))
				printk(KERN_INFO "Couldn't set task affinity\n");
			else
				printk(KERN_INFO "Pid:%d Affinity set on %d\n", task->pid, host_cpu);
		}
	}
}
