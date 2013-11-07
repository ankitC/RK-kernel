#ifndef SUSPENSION_FRAMEWORK_
#define SUSPENSION_FRAMEWORK_

void migrate_only(void);
void migrate_and_start(struct task_struct* task);
void wakeup_tasks(void);
long reserve_sched_setaffinity(pid_t pid, const struct cpumask *in_mask);
#endif
