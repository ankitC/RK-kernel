#include <linux/kernel.h>
#include <linux/linked_list.h>
#include <linux/nodefuncs.h>
#include <linux/bin_linked_list.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>

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
 *Adding a bin node
 */
void add_bin_node( BIN_NODE* curr1)
{
	BIN_NODE* curr2 = bin_head;
	BIN_NODE* temp = NULL;

	if (!curr1)
	{
		return;
	}

	if (bin_head == NULL)
	{
		printk(KERN_INFO "Bin Head Created");
		bin_head = curr1;
	}
	else
	{
		printk(KERN_INFO "Head bin Present");
		printk(KERN_INFO "curr2->u = %llu curr1->u = %llu\n", curr2->task->reserve_process.U \
				, curr1->task->reserve_process.U);

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

	printk(KERN_INFO "Deleting bin node\n");
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
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}
