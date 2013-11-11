#include <linux/kernel.h>
#include <linux/linked_list.h>
#include <linux/nodefuncs.h>
#include <linux/bin_linked_list.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>

extern struct mutex suspend_mutex;
extern struct semaphore wakeup_sem;
BIN_NODE *bin_head = NULL;

/*
 * Making a bin node 
 */
BIN_NODE* make_bin_node(struct task_struct *task)
{
	BIN_NODE* node = kmalloc(sizeof(BIN_NODE), GFP_KERNEL);
	node->task = task;
	node->next = NULL;
	return node;
}
/*
 * Finding if the node already exists in the linked list
 */
int find_bin_node(BIN_NODE* to_be_found)
{
	BIN_NODE* curr = bin_head;

	while (curr)
	{
		if (curr->task->pid == to_be_found->task->pid)
		{
			delete_bin_node(curr->task);
			add_bin_node(to_be_found);
			return 1;
		}
		curr = curr->next;
	}

	return 0;
}

/*
 *Adding a bin node
 */
void add_bin_node( BIN_NODE* curr1)
{
	BIN_NODE* curr2 = bin_head;
	BIN_NODE* temp = NULL;

	if (find_bin_node(curr1))
		return;

	if (!curr1)
	{
		return;
	}

	if (bin_head == NULL)
	{
		bin_head = curr1;
	}
	else
	{

		if (curr2->task->reserve_process.U < curr1->task->reserve_process.U)
		{
			curr1->next = curr2;
			bin_head = curr1;
		}
		else
		{
			while( curr2 && (curr2->task->reserve_process.U >=
				   	curr1->task->reserve_process.U) )
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
 * Delete a bin node
 */
void delete_bin_node (struct task_struct *task)
{
	BIN_NODE* curr = bin_head;
	BIN_NODE* prev = NULL;

	while (curr)
	{
		if (curr->task == task)
		{
			if (curr == bin_head)
			{
				bin_head = curr->next;
				kfree(curr);
			}
			else
			{
				prev->next = curr->next;
				kfree(curr);
			}
			curr = NULL;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}
