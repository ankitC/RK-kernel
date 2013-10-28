#ifndef __BIN_LINKED_LIST_H_
#define __BIN_LINKED_LIST_H_

typedef struct bin_list
{
	struct task_struct *task;
	struct bin_list *next;
}BIN_NODE;


void add_bin_node(BIN_NODE* curr1);
BIN_NODE* make_bin_node(struct task_struct* curr1);
void delete_bin_node (struct task_struct *task);

#endif
