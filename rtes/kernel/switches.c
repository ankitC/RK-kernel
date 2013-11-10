/*
 *File contains functions for the sysfs functionality
 */
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/sched.h>
#include <linux/bin_packing.h>
#include <linux/bin_linked_list.h>
#include <linux/energy_saving.h>
#include <linux/suspension_framework.h>
#include <asm/current.h>

int trace_ctx = 0, migrate = 0, disable_cpus = 0, guarantee = 0;
char partition_policy[2];
extern spinlock_t bin_spinlock;
extern BIN_NODE *bin_head;
extern struct mutex suspend_mutex;
volatile int suspend_processes = 0;
volatile int suspend_all = 0;
int wake_up_processes = 0;
/*
 * Function called when a read is done on sysfs util file
 */
static ssize_t switch_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	int var = 0;

	printk(KERN_INFO "Switch Show %s\n", attr->attr.name);

	if (strcmp(attr->attr.name, "partition_policy") == 0)
		return sprintf(buf, "%s\n", partition_policy);

	if (strcmp(attr->attr.name, "guarantee") == 0)
		var = guarantee;
	if (strcmp(attr->attr.name, "migrate") == 0)
		var = migrate;
	if (strcmp(attr->attr.name, "disable_cpus") == 0)
		var = disable_cpus;
	if (strcmp(attr->attr.name, "trace_ctx") == 0)
		var = trace_ctx;

	return sprintf(buf, "%d\n", var);
}

/*
 * Function called when a read is done on sysfs switch file
 */

static ssize_t switch_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	int var;
	char policy[2] = {0};
	int prev_migrate = 0, prev_guarantee = 0, retval = 0;

	printk(KERN_INFO "Switch Store %s\n", attr->attr.name);
	if (strcmp(attr->attr.name, "partition_policy") == 0)
	{
		strncpy(policy, buf, 1);
		if (guarantee)
		{
			if (apply_heuristic(policy))
			{
				strncpy(partition_policy, buf,1);
				if(migrate == 1)
					migrate_and_start(current);
				else
					migrate_only();
			}
		}
		else
		{
			strncpy(partition_policy, buf,1);
		}
		return count;
	}

	sscanf(buf, "%d", &var);

	if (strcmp(attr->attr.name, "guarantee") == 0)
	{
		prev_guarantee = guarantee;

		if (prev_guarantee == 0 && var == 1)
		{
			retval = apply_heuristic(partition_policy);
			if (bin_head != NULL && retval < 0)
			{
				printk(KERN_INFO "Guarntee did not succeed\n");
			}
			else
			{
				printk(KERN_INFO "Guarntee succeed with retval %d\n", retval);

				guarantee = var;
				if (bin_head != NULL && retval)
				{
					if(migrate == 1)
						migrate_and_start(current);
					else
						migrate_only();
				}
			}
		}
	}
	if (strcmp(attr->attr.name, "migrate") == 0)
	{
		prev_migrate = migrate;
		migrate = var;

		if ((prev_migrate == 0) && (migrate == 1))
		{
			mutex_lock(&suspend_mutex);
			suspend_all = 0;
			mutex_unlock(&suspend_mutex);
			wakeup_tasks();
		}

	}
	if (strcmp(attr->attr.name, "disable_cpus") == 0)
	{
		disable_cpus = var;
		if (var == 1)
			energy_savings();
	}
	if (strcmp(attr->attr.name, "trace_ctx") == 0)
		trace_ctx = var;

	return count;
}
struct kobj_attribute guarantee_attribute = __ATTR(guarantee, 0666, switch_show, switch_store);
struct kobj_attribute trace_ctx_attribute = __ATTR(trace_ctx, 0666, switch_show, switch_store);
struct kobj_attribute migrate_attribute = __ATTR(migrate, 0666, switch_show, switch_store);
struct kobj_attribute partition_policy_attribute = __ATTR(partition_policy, 0666, switch_show, switch_store);
struct kobj_attribute disable_cpus_attribute = __ATTR(disable_cpus, 0666, switch_show, switch_store);


struct attribute *attrs[] = {

	&guarantee_attribute.attr,
	&trace_ctx_attribute.attr,
	&migrate_attribute.attr,
	&partition_policy_attribute.attr,
	&disable_cpus_attribute.attr,   /* need to NULL terminate the list of attributes */
	NULL,
};

struct attribute_group attr_group = {
	.attrs = attrs,
};



/*
 * Creates pid directories and the util and overflow files
 */
int create_switches(struct kobject *config_obj)
{
	int retval = 0;
	partition_policy[0] = 'F';
	partition_policy[1] = '\0';
	retval = sysfs_create_group(config_obj, &attr_group);

	if(retval)
		kobject_put(config_obj);
	return retval;
}

