/*
 *  psdev.c: A simple loadable kernel module which gives the 
 *  information about the processes when read from.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/current.h>
#include <asm/uaccess.h>	/* for put_user */

#define SUCCESS 0
#define DEVICE_NAME "psdev"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 4096		/* Max length of the message from the device */

#define DRIVER_AUTHOR "Team_11"
#define DRIVER_DESC   "Char Device Driver Loadable Kernel module"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

/*
 *  Prototypes for the driver
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static loff_t device_seek(struct file *, loff_t, int);

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  
				 * Used to prevent multiple access to device */
static char *msg;	/* The msg the device will give when asked */
static char *msg_Ptr;

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
	.llseek = device_seek,
};


/*
 * This function is called when the module is loaded
 */
int init_module(void)
{
	/*
	 * Dynamically finding an available Major number for
	 * installing the device
	 */
    Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
	  printk(KERN_ALERT "Registering char device failed with %d\n", Major);
	  return Major;
	}

	printk(KERN_INFO "Create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);

	return SUCCESS;
}

void cleanup_module(void)
{
	/* 
	 * Unregister the device 
	 */
	printk(KERN_DEBUG "Cleaning up the module.\n");
	unregister_chrdev(Major, DEVICE_NAME);
}

/* 
 *  * Called when a process tries to open the device file.
 */
static int device_open(struct inode *inode, struct file *file)
{

	struct task_struct *task;
	char* null_char= '\0';

	if (Device_Open)
		return -EBUSY;

	Device_Open++;
	msg = kmalloc(BUF_LEN, GFP_KERNEL);
	sprintf(msg,"pid\tpr\tname\n");

	/*Reading the task list*/
	read_lock(&tasklist_lock);

	for_each_process (task)
	{
		sprintf(msg, "%s %d\t%d\t%s\n", msg, task->pid, task->prio, task->comm);
	}

	read_unlock(&tasklist_lock);
	sprintf(msg,"%s%s", msg, null_char);
	msg_Ptr = msg;
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/* 
 *  * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;
	kfree(msg);
	/* 
	 *	* Decrement the usage count, or else once you opened the file.
	 */
	module_put(THIS_MODULE);

	return SUCCESS;
}

/* 
 *  * Called when a process, which already opened the dev file, attempts to
 *  * read from it.
 */
static ssize_t device_read(struct file *filp,
		char *buffer,	/* buffer to fill with data */
		size_t length,	/* length of the buffer     */
		loff_t * offset)
{
	/*
	 *	 * Number of bytes written to the buffer 
	 */
	int bytes_read = 0;
	/*
	 *  If we're at the end of the message, 
	 *  return 0 signifying end of file 
	 */
	if (*msg_Ptr == 0)
		return 0;

	/* 
	 *	 Write the data to the buffer.
	 */
	while (length && *msg_Ptr) {

		/* 
		 * Using the put_user instead of copy_to
		 * to copy a character at a time.
		 */
		put_user(*(msg_Ptr++), buffer++);

		length--;
		bytes_read++;
	}

	/* 
	 *	 *return the number of bytes put into the buffer
	 */
	return bytes_read;
}

/*  
 *   * Called when a process writes to dev file,
 *	 * since we do not do anything, we print the message to 
 *	 * the kernel and return.
 */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
	return -EINVAL;
}

static loff_t
device_seek(struct file *filep, loff_t pos, int len)
{
	printk(KERN_ALERT "Seek called, but not supported.\n");
	return -EINVAL;
}
