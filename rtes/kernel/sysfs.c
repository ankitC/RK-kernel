/*
 *File contains functions for the sysfs functionality
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/sysfs_func.h>

/*
 * Function called when a read is done on sysfs util file
 */
static ssize_t util_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	struct reserve_obj* reservation_detail = container_of(attr, \
			struct reserve_obj, util_attr);
	int len = circular_buffer_read(reservation_detail, buf);

	return len;
}

/*
 * Function called when a read is done on sysfs overflow file
 */
static ssize_t overflow_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	struct reserve_obj* reservation_detail = container_of(attr, \
			struct reserve_obj, overflow_attr);

	return sprintf(buf, "%d\n", reservation_detail->buffer_overflow);
}

/*
 * Function called when a read is done on sysfs tval file
 */
static ssize_t tval_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	struct reserve_obj* reservation_detail = container_of(attr, \
			struct reserve_obj, tval_attr);

	return sprintf(buf, "%llu\n", timespec_to_ns(&reservation_detail->T));
}
/*
 * Function called when a read is done on sysfs ctx file
 */
static ssize_t ctx_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	struct reserve_obj* reservation_detail = container_of(attr, \
			struct reserve_obj, ctx_attr);
	int len = ctx_buffer_read(reservation_detail, buf);

	return len;

}
/*
 * Function called when a read is done on sysfs energy file
 */
static ssize_t energy_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	struct reserve_obj* reservation_detail = container_of(attr, \
			struct reserve_obj, energy_attr);
	int len = energy_buffer_read(reservation_detail, buf);

	return len;

}

/*
 * Function called when a read is done on sysfs total_energy file
 */
static ssize_t total_energy_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
//	int len = energy_buffer_read(reservation_detail, buf);
 int len = 0;
	return len;

}

struct kobject *tasks_kobj;
struct kobj_attribute total_energy_attribute = __ATTR(energy, 0666, total_energy_show, NULL);


struct attribute *attrs_energy[] = {

	&total_energy_attribute.attr,
	NULL,
};

struct attribute_group attr_energy_group = {
	.attrs = attrs_energy,
};


/*
 * Creates initial directories rtes, tasks for sysfs
 */
int create_directories(void)
{
	static struct kobject *rtes_kobj, *config_kobj;
	printk(KERN_INFO "creating initial directories\n");
	if(!(rtes_kobj = kobject_create_and_add("rtes", NULL)))
		return -ENOMEM;

	if(!(tasks_kobj = kobject_create_and_add("tasks", rtes_kobj)))
		return -ENOMEM;

	if(!(config_kobj = kobject_create_and_add("config", rtes_kobj)))
		return -ENOMEM;
	
	if(sysfs_create_group(rtes_kobj, &attr_energy_group))
		kobject_put(rtes_kobj);

	//if(!(config_kobj = kobject_create_and_add("energy", rtes_kobj)))
	//	return -ENOMEM;

	create_switches(config_kobj);
	return 0;
}
/*
 * Creates pid directories and the util and overflow files
 */
int create_pid_dir_and_reserve_file(struct task_struct *task)
{
	int retval = 0;
	pid_t pid;
	char pid_directory[16];
	struct kobj_attribute util_attribute = __ATTR(util, 0666, util_show, NULL);
	struct kobj_attribute overflow_attribute = __ATTR(overflow, 0666, overflow_show, NULL);
	struct kobj_attribute tval_attribute = __ATTR(tval, 0666, tval_show, NULL);
	struct kobj_attribute ctx_attribute = __ATTR(ctx, 0666, ctx_show, NULL);
	struct kobj_attribute energy_attribute = __ATTR(energy, 0666, energy_show, NULL);

	struct attribute_group attr_group = {
		 .attrs = task->reserve_process.attrs,
	};

	pid = task->pid;
	sprintf(pid_directory,"%d", pid);
	if(!(task->reserve_process.pid_obj = \
				kobject_create_and_add(pid_directory, tasks_kobj)))
		return -ENOMEM;

	task->reserve_process.util_attr = util_attribute;
	task->reserve_process.overflow_attr = overflow_attribute;
	task->reserve_process.tval_attr = tval_attribute;
	task->reserve_process.ctx_attr = ctx_attribute;
	task->reserve_process.energy_attr = energy_attribute;

	task->reserve_process.attrs[0] = &task->reserve_process.util_attr.attr;
	task->reserve_process.attrs[1] = &task->reserve_process.overflow_attr.attr;
	task->reserve_process.attrs[2] = &task->reserve_process.tval_attr.attr;
	task->reserve_process.attrs[3] = &task->reserve_process.ctx_attr.attr;
	task->reserve_process.attrs[4] = &task->reserve_process.energy_attr.attr;
	task->reserve_process.attrs[5] = NULL;

	retval = sysfs_create_group(task->reserve_process.pid_obj, &attr_group);

	if(retval)
		kobject_put(tasks_kobj);
	return retval;
}
/*
 * Removes pid directories and the util and overflow files
 */

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

/*
 * Circular Buffer function where data is written to circular buffer
 */
void circular_buffer_write(struct reserve_obj* res_detail, struct timespec spent_budget)
{
	circular_buffer *c_buffer = &res_detail->c_buf;
	unsigned long long time = timespec_to_ns(&spent_budget);
	char time_buffer[36];
	struct timespec ts;
	int len = 0, i = 0;

	getrawmonotonic(&ts);
	sprintf(time_buffer, "%llu %llu", time, timespec_to_ns(&ts));
	len = strlen(time_buffer) + 1;
	while(len)
	{
		*(c_buffer->buffer + (c_buffer->end % PAGE_SIZE)) = time_buffer[i];
		c_buffer->end = ((c_buffer->end+1) % PAGE_SIZE);
		i++;
		len = len - 1;

		if (c_buffer->end == c_buffer->start)
		{
			res_detail->buffer_overflow = 1;
		}
	}

	if (res_detail->buffer_overflow)
	{
		if( (c_buffer->start = c_buffer->end - strlen(time_buffer) - 1) < 0)
		{
			c_buffer->start = PAGE_SIZE + c_buffer->start;
		}
	}

	if (c_buffer->start < c_buffer->end)
		c_buffer->read_count = c_buffer->end - c_buffer->start;
	else
	{
		c_buffer->read_count = (c_buffer->end - 0)+ (PAGE_SIZE -1  - c_buffer->start);
	}
//	printk(KERN_INFO "WRITE --->Pid=%d len = %d Buffer %s Start %d  End %d read_count %d\n",res_detail->monitored_process->pid,  strlen(time_buffer)+1, time_buffer,c_buffer->start, c_buffer->end, c_buffer->read_count);
	
}
/*
 * Circular Buffer function where data is read to circular buffer
 */

int circular_buffer_read(struct reserve_obj* res_detail , char* buf)
{
	int len = 0, i = 0;
	circular_buffer *c_buffer = &res_detail->c_buf;

	if (c_buffer->read_count <= 0)
	{
		buf = NULL;
		return 0;
	}

	if (res_detail->buffer_overflow == 1)
	{
		res_detail->buffer_overflow = 0;
	}

	while( *(c_buffer->buffer + c_buffer->start))
	{
		buf[i] = *(c_buffer->buffer + (c_buffer->start % PAGE_SIZE));
		c_buffer->start = ((c_buffer->start+1) % PAGE_SIZE);
		i++;
		len++;
	}

	c_buffer->start = ((c_buffer->start+1) % PAGE_SIZE);
	c_buffer->read_count -= (len + 1);

//	printk(KERN_INFO "READ ---> Buffer %s Pid %d Start %d End %d Len of buffer %d read_count %d\n", buf, res_detail->monitored_process->pid, c_buffer->start, c_buffer->end, len, c_buffer->read_count);
	return len;
}

/*
 * Circular Buffer function where data is written to ctx circular buffer
 */
void ctx_buffer_write(struct reserve_obj* res_detail, struct timespec spent_budget, int ctx_in)
{
	circular_buffer *c_buffer = &res_detail->ctx_buf;
	unsigned long long time = timespec_to_ns(&spent_budget);
	char time_buffer[33];
	int len = 0, i = 0;
	if (ctx_in)
		sprintf(time_buffer, "%llu %s", time, "in");
	else
		sprintf(time_buffer, "%llu %s", time, "out");

	len = strlen(time_buffer) + 1;
	while(len)
	{
		*(c_buffer->buffer + (c_buffer->end % PAGE_SIZE)) = time_buffer[i];
		c_buffer->end = ((c_buffer->end+1) % PAGE_SIZE);
		i++;
		len = len - 1;
	
		if (c_buffer->end == c_buffer->start)
		{
			res_detail->ctx_overflow = 1;
		}
	}

	if (res_detail->ctx_overflow)
	{
		if( (c_buffer->start = c_buffer->end - strlen(time_buffer) - 1) < 0)
		{
			c_buffer->start = PAGE_SIZE + c_buffer->start;
		}
	}

	if (c_buffer->start < c_buffer->end)
		c_buffer->read_count = c_buffer->end - c_buffer->start;
	else
	{
		c_buffer->read_count = (c_buffer->end - 0)+ (PAGE_SIZE - 1 - c_buffer->start);
	}
	//printk(KERN_INFO "WRITE --->Pid=%d len = %d Buffer %s Start %d  End %d read_count %d\n",res_detail->monitored_process->pid,  strlen(time_buffer)+1, time_buffer,c_buffer->start, c_buffer->end, c_buffer->read_count);
	
}
/*
 * Circular Buffer function where data is read to ctx circular buffer
 */

int ctx_buffer_read(struct reserve_obj* res_detail , char* buf)
{
	int len = 0, i = 0;
	circular_buffer *c_buffer = &res_detail->ctx_buf;
	
	if (c_buffer->read_count <= 0)
	{
		buf = NULL;
		return 0;
	}

	if (res_detail->ctx_overflow == 1)
	{
		res_detail->ctx_overflow = 0;
	}

	while( *(c_buffer->buffer + c_buffer->start))
	{
		buf[i] = *(c_buffer->buffer + (c_buffer->start % PAGE_SIZE));
		c_buffer->start = ((c_buffer->start+1) % PAGE_SIZE);
		i++;
		len++;
	}

	c_buffer->start = ((c_buffer->start+1) % PAGE_SIZE);
	c_buffer->read_count -= (len + 1);

	//printk(KERN_INFO "READ ---> Buffer %s Pid %d Start %d End %d Len of buffer %d read_count %d\n", buf, res_detail->monitored_process->pid, c_buffer->start, c_buffer->end, len, c_buffer->read_count);
	return len;
}
/*
 * Circular Buffer function where data is written to energy circular buffer
 */
void energy_buffer_write(struct reserve_obj* res_detail, struct timespec spent_budget)
{
	circular_buffer *c_buffer = &res_detail->energy_buf;
	char time_buffer[33];
	int len = 0, i = 0;

	len = strlen(time_buffer) + 1;
	while(len)
	{
		*(c_buffer->buffer + (c_buffer->end % PAGE_SIZE)) = time_buffer[i];
		c_buffer->end = ((c_buffer->end+1) % PAGE_SIZE);
		i++;
		len = len - 1;

		if (c_buffer->end == c_buffer->start)
		{
			res_detail->energy_overflow = 1;
		}
	}

	if (res_detail->energy_overflow)
	{
		if( (c_buffer->start = c_buffer->end - strlen(time_buffer) - 1) < 0)
		{
			c_buffer->start = PAGE_SIZE + c_buffer->start;
		}
	}

	if (c_buffer->start < c_buffer->end)
		c_buffer->read_count = c_buffer->end - c_buffer->start;
	else
	{
		c_buffer->read_count = (c_buffer->end - 0)+ (PAGE_SIZE - 1 - c_buffer->start);
	}
	//printk(KERN_INFO "WRITE --->Pid=%d len = %d Buffer %s Start %d  End %d read_count %d\n",res_detail->monitored_process->pid,  strlen(time_buffer)+1, time_buffer,c_buffer->start, c_buffer->end, c_buffer->read_count);
	
}
/*
 * Circular Buffer function where data is read to energy circular buffer
 */

int energy_buffer_read(struct reserve_obj* res_detail , char* buf)
{
	int len = 0, i = 0;
	circular_buffer *c_buffer = &res_detail->ctx_buf;
	
	if (c_buffer->read_count <= 0)
	{
		buf = NULL;
		return 0;
	}

	if (res_detail->energy_overflow == 1)
	{
		res_detail->energy_overflow = 0;
	}

	while( *(c_buffer->buffer + c_buffer->start))
	{
		buf[i] = *(c_buffer->buffer + (c_buffer->start % PAGE_SIZE));
		c_buffer->start = ((c_buffer->start+1) % PAGE_SIZE);
		i++;
		len++;
	}

	c_buffer->start = ((c_buffer->start+1) % PAGE_SIZE);
	c_buffer->read_count -= (len + 1);

	//printk(KERN_INFO "READ ---> Buffer %s Pid %d Start %d End %d Len of buffer %d read_count %d\n", buf, res_detail->monitored_process->pid, c_buffer->start, c_buffer->end, len, c_buffer->read_count);
	return len;
}
