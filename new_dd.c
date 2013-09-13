/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/uaccess.h>	/* for put_user */

/*  
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);

#define SUCCESS 0
#define DEVICE_NAME "psdev"	/* Dev name as it appears in /proc/devices   */

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  
							 * Used to prevent multiple access to device */

static struct file_operations fops = {
	.open = device_open,
	.release = device_release
};

/*
 * This function is called when the module is loaded
 */
int init_module(void)
{
	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
		printk(KERN_ALERT "Registering char device failed with %d\n", Major);
		return Major;
	}

	printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");


	return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
	/* 
	 * Unregister the device 
	 */
	unregister_chrdev(Major, DEVICE_NAME);
	/*if (ret < 0)
	  printk(KERN_ALERT "Error in unregister_chrdev: %d\n", ret);
	  */
}

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
	
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;		/* We're now ready for our next caller */

	/* 
	 * Decrement the usage count, or else once you opened the file, you'll
	 * never get get rid of the module. 
	 */
	module_put(THIS_MODULE);

	return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */

static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
		char *buffer,	/* buffer to fill with data */
		size_t length,	/* length of the buffer     */
		loff_t * offset)
{
	struct task_struct *task;

	printk(KERN_DEBUG "pid\tpr\tname");

	for_each_process (task)
	{
		printk(KERN_DEBUG "%i\t%i\t%s", task->pid, task->prio, task->comm);
	}
	printk(KERN_DEBUG "read");
	return 0;
}

