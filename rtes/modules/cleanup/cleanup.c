#include <linux/init.h>
#include <linux/module.h> 
#include<linux/sched.h>
#include <linux/rcupdate.h>
#include <linux/fdtable.h>
#include <linux/fs.h> 
#include <linux/fs_struct.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <asm/unistd.h>
#include <asm/current.h>


MODULE_LICENSE("GPL");

static char* comm = "";
module_param(comm, charp, 0);
MODULE_PARM_DESC(mystring, "A character string");

unsigned long **sys_call_table = (unsigned long **) 0xc000ec84;
asmlinkage int (*original_sys_exit)(int);
asmlinkage int (*original_sys_exit_group)(int);

asmlinkage int our_fake_exit_group_function(int error_code)
{

	struct files_struct *current_files; 
	struct fdtable *files_table;
	int i=0;
	struct path files_path;
	char *cwd;
	char *buf = (char *)kmalloc(GFP_KERNEL,100*sizeof(char));

	printk(KERN_ALERT "INSIDE FAKE EXIT_GROUP");
	printk(KERN_ALERT "Name of the current task  %s\n", current->comm);
	current_files = current->files;
	files_table = files_fdtable(current_files);

	if (strstr (current->comm, comm))
	{
		while(files_table->fd[i] != NULL)
		{ 
			files_path = files_table->fd[i]->f_path;
			cwd = d_path(&files_path,buf,100*sizeof(char));

			printk(KERN_ALERT "Open file with fd %d  %s", i, cwd);

			i++;
		}/*call original sys_exit and return its value*/}
	return original_sys_exit_group(error_code);
}
asmlinkage int our_fake_exit_function(int error_code)
{

	struct files_struct *current_files; 
	struct fdtable *files_table;
	int i=0;
	struct path files_path;
	char *cwd;
	char *buf = (char *)kmalloc(GFP_KERNEL,100*sizeof(char));

	printk(KERN_ALERT "INSIDE FAKE EXIT");
	printk(KERN_ALERT "Name of the current task  %s\n", current->comm);
	current_files = current->files;
	files_table = files_fdtable(current_files);

	if (strstr (current->comm, comm))
	{
		while(files_table->fd[i] != NULL)
		{ 
			files_path = files_table->fd[i]->f_path;
			cwd = d_path(&files_path,buf,100*sizeof(char));

			printk(KERN_ALERT "Open file with fd %d  %s", i, cwd);

			i++;
		}/*call original sys_exit and return its value*/}
	return original_sys_exit(error_code);
}

int init_module(void)
{

	/*store reference to the original sys_exit call*/
	original_sys_exit = (void *) sys_call_table[__NR_exit];
	original_sys_exit_group = (void *) sys_call_table[__NR_exit_group];

	/*manipulate sys_call_table to call our fake exit
	 *     function instead*/
	sys_call_table[__NR_exit] = (unsigned long* ) our_fake_exit_function;
	sys_call_table[__NR_exit_group] = (unsigned long* ) our_fake_exit_group_function;
	return 0;
}

void cleanup_module(void)
{
	/*restore original sys_exit*/
	sys_call_table[__NR_exit] = (unsigned long* ) original_sys_exit;
	sys_call_table[__NR_exit_group] = (unsigned long* ) original_sys_exit_group;
}
