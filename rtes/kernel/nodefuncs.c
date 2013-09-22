/*Our custom linked list*/

#include <linux/kernel.h>
#include <linux/nodefuncs.h>
#include <linux/fs.h>

#define D(x) x

/*Addind a node in a sorted order of descending priorities*/
void add_node(TASK_NODE** head, TASK_NODE* curr1)
{
	TASK_NODE* curr2 = *head;
	TASK_NODE* temp = NULL;

	if (!curr1)
	{
		return;
	}

	if (*head == NULL)
	{
		D(printk(KERN_INFO "Head Created"));
		*head = curr1;
	}
	else
	{
		if (curr2->prio <= curr1->prio)
		{
			curr1->next = curr2;
			*head = curr1;
			D(printk(KERN_INFO "Node Added"));
		}
		else
		{
			while( curr2 && (curr2->prio > curr1->prio) ){
				temp=curr2;
				curr2= curr2->next;
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
void delete_linked_list (TASK_NODE** head)
{
	TASK_NODE* curr = *head;

	while (curr)
	{
		TASK_NODE* temp = curr;
		curr = curr->next;
		kfree(temp);
		temp = NULL;
	}
}
