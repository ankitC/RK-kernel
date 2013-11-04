#ifndef SUSPENSION_FRAMEWORK_
#define SUSPENSION_FRAMEWORK_

void migrate_only(void);
void migrate_and_start(struct task_struct* task);
void wakeup_tasks(void);
#endif
