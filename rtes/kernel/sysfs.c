#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/sched.h>
#include <asm/current.h>
//static int y=0;
static ssize_t util_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	struct reserve_obj* reservation_detail = container_of(attr, \
			struct reserve_obj, util_attr);
	int len = circular_buffer_read(reservation_detail, buf);

	return len;
}

static ssize_t overflow_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	struct reserve_obj* reservation_detail = container_of(attr, \
			struct reserve_obj, overflow_attr);

	return sprintf(buf, "%d\n", reservation_detail->buffer_overflow);
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
	struct kobj_attribute overflow_attribute = __ATTR(overflow, 0666, overflow_show, NULL);

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

	task->reserve_process.attrs[0] = &task->reserve_process.util_attr.attr;
	task->reserve_process.attrs[1] = &task->reserve_process.overflow_attr.attr;
	task->reserve_process.attrs[2] = NULL;

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


void circular_buffer_write(struct reserve_obj* res_detail, struct timespec spent_budget)
{
	circular_buffer *c_buffer = &res_detail->c_buf;
	unsigned long long time = timespec_to_ns(&spent_budget);
	char time_buffer[21];
	int len = 0, i = 0;
	sprintf(time_buffer, "%llu", time);
	len = strlen(time_buffer) + 1;
	while(len)
	{
		//*(c_buffer->buffer + (c_buffer->end % PAGE_SIZE)) = time_buffer[i];
		*(c_buffer->buffer + (c_buffer->end % 96)) = time_buffer[i];
		//c_buffer->end = (c_buffer->end % PAGE_SIZE) + 1;
		c_buffer->end = ((c_buffer->end+1) % 96);
		i++;
		len = len - 1;
		
		if (c_buffer->end == c_buffer->start)
		{
			printk(KERN_INFO "Overflow occured\n");
			res_detail->buffer_overflow = 1;
		}
	}

	if (res_detail->buffer_overflow)
	{
		//		int temp = c_buffer->start;
		if( (c_buffer->start = c_buffer->end - strlen(time_buffer) - 1) < 0)
		{
			c_buffer->start = 96 + c_buffer->start;
		}
	}

//	c_buffer->read_count += strlen(time_buffer)+1;
	if (c_buffer->start < c_buffer->end)
		c_buffer->read_count = c_buffer->end - c_buffer->start;
	else
	{
		c_buffer->read_count = (c_buffer->end - 0)+ (95 - c_buffer->start);
	}
	printk(KERN_INFO "WRITE --->Pid=%d len = %d Buffer %s Start %d  End %d read_count %d\n",res_detail->monitored_process->pid,  strlen(time_buffer)+1, time_buffer,c_buffer->start, c_buffer->end, c_buffer->read_count);
	
}

int circular_buffer_read(struct reserve_obj* res_detail , char* buf)
{
	int len = 0, i = 0;
	circular_buffer *c_buffer = &res_detail->c_buf;
//	printk(KERN_INFO "Reading from circular buffer\n");

	if (c_buffer->read_count <= 0)
	{
		printk(KERN_INFO "Read count is lesser than zero \n");
		buf = NULL;
		return 0;
	}
	/*if (*c_buffer->buffer == 0)
	{
		buf[i] = 0;
		printk(KERN_INFO "Buffer is null returning 0\n");
		return 0;
	}*/
	if (res_detail->buffer_overflow == 1)
	{
		res_detail->buffer_overflow = 0;
//		c_buffer->read_count = 0;
	}

	while( *(c_buffer->buffer + c_buffer->start))
	{
		//buf[i] = *(c_buffer->buffer + (c_buffer->start % PAGE_SIZE));
		buf[i] = *(c_buffer->buffer + (c_buffer->start % 96));
		//c_buffer->start = (c_buffer->start % PAGE_SIZE) + 1;
		c_buffer->start = ((c_buffer->start+1) % 96);
		i++;
		len++;
	}
	//c_buffer->start = (c_buffer->start % PAGE_SIZE) + 1;
	c_buffer->start = ((c_buffer->start+1) % 96);
	c_buffer->read_count -= (len + 1);
//	printk(KERN_INFO "Reading from circular buffer %s\n", );
	printk(KERN_INFO "READ ---> Buffer %s Pid %d Start %d End %d Len of buffer %d read_count %d\n", buf, res_detail->monitored_process->pid, c_buffer->start, c_buffer->end, len, c_buffer->read_count);
	return len;
}
