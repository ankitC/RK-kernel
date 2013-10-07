#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/sched.h>
#include <asm/current.h>

static ssize_t util_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	int g =26342346;
	return sprintf(buf, "%d",g );
}

struct kobject *tasks_kobj;
/*static struct attribute * attrs [] =
{
	&util_attribute.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

*/
int create_directories(void)
{
	static struct kobject *rtes_kobj;
	printk(KERN_INFO "creating initial directories\n");
	if(!(rtes_kobj = kobject_create_and_add("rtes", NULL)))
		return -ENOMEM;

	if(!(tasks_kobj = kobject_create_and_add("tasks", rtes_kobj)))
		return -ENOMEM;
	return 0;
}
int create_pid_dir_and_reserve_file(struct task_struct *task)
{
	int retval = 0;
	pid_t pid;
	char pid_directory[16];
	struct kobj_attribute util_attribute = __ATTR(util, 0666, util_show, NULL);

	struct attribute_group attr_group = {
		 .attrs = task->reserve_process.attrs,
	};

	pid = task->pid;
	sprintf(pid_directory,"%d", pid);
	if(!(task->reserve_process.pid_obj = kobject_create_and_add(pid_directory, tasks_kobj)))
		return -ENOMEM;

	task->reserve_process.util_attr = util_attribute;//__ATTR(util, 0666, util_show, NULL);

	task->reserve_process.attrs[0] = &task->reserve_process.util_attr.attr;
	task->reserve_process.attrs[1] = NULL;

	retval = sysfs_create_group(task->reserve_process.pid_obj, &attr_group);

	if(retval)
		kobject_put(tasks_kobj);
	return retval;
}

void remove_pid_dir_and_reserve_file(struct task_struct *task)
{
	char pid_directory[16];

	struct attribute_group attr_group = {
		 .attrs = task->reserve_process.attrs,
	};
	sprintf(pid_directory,"%d", task->pid);

	sysfs_remove_group(task->reserve_process.pid_obj, &attr_group);
	kobject_put(task->reserve_process.pid_obj);
	return;
}
