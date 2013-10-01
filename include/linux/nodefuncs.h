#ifndef NODEFUNCS_H
#define NODEFUNCS_H

typedef struct task_node
{
	int pid;
	int prio;
	char name[32];
	struct task_node* next;

}TASK_NODE;
void add_node(TASK_NODE**, TASK_NODE*);
void delete_linked_list (TASK_NODE**);

#endif /* NODEFUNCS_H */
