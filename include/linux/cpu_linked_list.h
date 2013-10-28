#ifndef __CPU_LINKED_LIST_H_
#define __CPU_LINKED_LIST_H_

#include <linux/bin_linked_list.h>

void add_cpu_node(BIN_NODE* curr1, int cpu);
BIN_NODE* make_cpu_node(struct task_struct* curr1);
void delete_cpu_node (struct task_struct *task, int cpu);
void delete_all_cpu_nodes (void);

#endif
