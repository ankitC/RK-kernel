#ifndef __LINKED_LIST_H_
#define __LINKED_LIST_H_

typedef struct proc_list
{
	struct task_struct *task;
	struct proc_list *next;
}PROC_NODE;


void add_ll_node(PROC_NODE* curr1);
PROC_NODE* make_node(struct task_struct* curr1);
void delete_node (struct task_struct *task);

#endif
