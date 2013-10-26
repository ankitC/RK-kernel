#include <linux/kernel.h>
#include <linux/linked_list.h>
#include <linux/nodefuncs.h>
#include <linux/fs.h>
#include <linux/slab.h>
extern PROC_NODE * head;
PROC_NODE* make_node(struct task_struct *task)
{
	PROC_NODE* node = kmalloc(sizeof(PROC_NODE), GFP_KERNEL);
	node->task = task;
	node->next = NULL;
	return node;
}

/*Addind a node*/
void add_ll_node( PROC_NODE* curr1)
{
	PROC_NODE* curr2 = head;

	if (!curr1)
	{
		return;
	}

	if (head == NULL)
	{
		printk(KERN_INFO "Head Created");
		head = curr1;
	}
	else
	{
		curr1->next = curr2;
		head = curr1;
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
