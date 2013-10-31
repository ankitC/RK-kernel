#ifndef _BIN_PACKING__
#define _BIN_PACKING__

#include <linux/bin_linked_list.h>

int ub_cpu_test(BIN_NODE *curr1, int cpu);
int apply_heuristic(char p[2]);
void wake_up_tasks(void);

#endif

