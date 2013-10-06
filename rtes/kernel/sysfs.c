#include <linux/kobject.h>


extern struct kobejct *kernel_kobj;
struct kobject *task_obj;
/**
 * device_create_file - create sysfs attribute file for device.
 * @dev: device.
 * @attr: device attribute descriptor.
 */
int device_create_file(struct device *dev,
		       const struct device_attribute *attr)
{
	int error = 0;
	if (dev)
		error = sysfs_create_file(&dev->kobj, &attr->attr);
	return error;
}
static const struct sysfs_ops dev_sysfs_ops = {
	.show	= dev_attr_show,
	.store	= dev_attr_store,
};


static int create_directories;(void)
{
	static struct kobject *rtes_kobj;
	printk(KERN_INFO "creating directories");
	if(!(rtes_kobj = kobject_create_and_add("rtes", kernel_kobj)))
		return -ENOMEM;

	if(!(task_kobj = kobject_create_and_add("tasks", rtes_kobj)))
		return -ENOMEM;
}

int create_pid_dir_and_reserve_file(pid_t pid)
{
	struct kobject *pid_obj;
	char pid_directory[16];
	sprintf(pid_directory,"%d", pid);
	if(!(pid_obj = kobject_create_and_add(pid_directory, task_obj)))
		return -ENOMEM;
	retval = sysfs_create_file(ex_kobj, &attr_group;
	if(retval)
		kobject_put(pid_directory);
	return retval;


}

static ssize_t util_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	return sprintf(buf, "%dn", foo);
}

static struct kobj_attribute foo_attribute = __ATTR(foo, 0666, foo_show, foo_store);

static struct attribute * attrs [] =
{
	&foo_attribute.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

}

static void __exit ex_exit(void)
{
	kobject_put(ex_kobj);
	printk(KERN_INFO "i am unloading...n");
}
