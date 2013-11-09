#include <linux/kernel.h>
#include <linux/linked_list.h>
#include <linux/nodefuncs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>

extern PROC_NODE * head;
PROC_NODE* make_node(struct task_struct *task)
{
	PROC_NODE* node = kmalloc(sizeof(PROC_NODE), GFP_KERNEL);
	node->task = task;
	node->next = NULL;
	return node;
}
/*
 * Finding if the node already exists in the linked list
 */
int find_proc_node(PROC_NODE* to_be_found)
{
	PROC_NODE* curr = head;

	while (curr)
	{
		if (curr->task->pid == to_be_found->task->pid)
		{
			delete_node(curr->task);
			add_ll_node(to_be_found);
			return 1;
		}
		curr = curr->next;
	}

	return 0;
}
/*Addind a node*/
void add_ll_node( PROC_NODE* curr1)
{
	PROC_NODE* curr2 = head;
	PROC_NODE* temp = NULL;

	if (find_proc_node(curr1))
		return;

	if (!curr1)
	{
		return;
	}

	if (head == NULL)
	{
		head = curr1;
	}
	else
	{
		printk(KERN_INFO "curr2->T = %llu curr1->T = %llu\n", timespec_to_ns(&curr2->task->reserve_process.T) \
				, timespec_to_ns(&curr1->task->reserve_process.T));
		if (timespec_to_ns(&curr2->task->reserve_process.T) >
				   	timespec_to_ns(&curr1->task->reserve_process.T))
		{
			curr1->next = curr2;
			head = curr1;
		}
		else
		{
			while( curr2 && (timespec_to_ns(&curr2->task->reserve_process.T) <=
				   	timespec_to_ns(&curr1->task->reserve_process.T) ))
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

/*Delete the constructed linked list*/
void delete_node (struct task_struct *task)
{
	PROC_NODE* curr = head;
	PROC_NODE* prev = NULL;

	printk(KERN_INFO "Deleting node\n");
	while (curr)
	{
		if (curr->task == task)
		{
			if (curr == head)
			{
				head = curr->next;
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
