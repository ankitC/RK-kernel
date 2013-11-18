#include <linux/kernel.h>
#include <linux/linked_list.h>
#include <linux/cpu_linked_list.h>
#include <linux/bin_linked_list.h>
#include <linux/nodefuncs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>


extern BIN_NODE* cpu_bin_head[4];
/*
 * Making a cpu node 
 */
BIN_NODE* make_cpu_node(struct task_struct *task)
{
	BIN_NODE* node = kmalloc(sizeof(BIN_NODE), GFP_KERNEL);
	node->task = task;
	node->next = NULL;
	return node;
}

/*
 *Adding a cpu node
 */
void add_cpu_node( BIN_NODE* curr1, int cpu)
{
	BIN_NODE* curr2 = cpu_bin_head[cpu];
	BIN_NODE* temp = NULL;

	if (!curr1)
	{
		return;
	}

	if (cpu_bin_head[cpu] == NULL)
	{
		cpu_bin_head[cpu] = curr1;
	}
	else
	{

		if (timespec_to_ns(&curr2->task->reserve_process.T) > timespec_to_ns(&curr1->task->reserve_process.T))
		{
			curr1->next = curr2;
			cpu_bin_head[cpu] = curr1;
		}
		else
		{
			while( curr2 && (timespec_to_ns(&curr2->task->reserve_process.T) <=
				   	timespec_to_ns(&curr1->task->reserve_process.T)) )
			{
				temp = curr2;
				curr2 = curr2->next;
			}

			if(curr2)
			{
				curr1->next = curr2;
			}
			temp->next = curr1;
		}
	}
}

/*
 * Delete a cpu node
 */
void delete_cpu_node (struct task_struct *task, int cpu)
{
	BIN_NODE* curr = cpu_bin_head[cpu];
	BIN_NODE* prev = NULL;

	while (curr)
	{
		if (curr->task == task)
		{
			if (curr == cpu_bin_head[cpu])
			{
				cpu_bin_head[cpu] = curr->next;
				kfree(curr);
			}
			else
			{
				prev->next = curr->next;
				kfree(curr);
			}
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}

/*
 * Deleting all cpu linked lists
 */
void delete_all_cpu_nodes(void)
{
	BIN_NODE* temp = NULL;
	BIN_NODE* curr = NULL;
	int i;

	for( i = 0; i < 4; i++)
	{
		curr = cpu_bin_head[i];

		while(curr)
		{
			temp = curr;
			curr = curr->next;
			kfree(temp);
			temp = NULL;
		}

		cpu_bin_head[i] = NULL;
	}
}
