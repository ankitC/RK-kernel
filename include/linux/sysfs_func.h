#ifndef SYSFS_FUNC_H
#define SYSFS_FUNC_H

int create_directories(void);
void create_switches(struct kobject *config);
int create_pid_dir_and_reserve_file(struct task_struct *task);
void remove_pid_dir_and_reserve_file(struct task_struct *task);
#endif
