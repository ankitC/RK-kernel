#ifndef PARTITION_SCHEDULING_H
#define PARTITION_SCHEDULING_H

void set_cpu_for_task(struct task_struct *task);
int admission_test(struct task_struct *task);

#endif
