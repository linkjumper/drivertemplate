#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>  /*copy_to_user()*/
#include <linux/wait.h>   /*wait_queues*/
#include <linux/random.h> /*get_random_bytes*/

#define MEMBUF		128
#define ATOMIC_BUF	10	
#define MODULNAME	"template"
#define VERSION_NR	"V0.2"
#define RELEASE_DATE	"2017-03-07"
#define AUTHOR		"linkjumper"
#define FILENAME	"mod.c"
#define VERSION 	"Id:"FILENAME" "VERSION_NR" "RELEASE_DATE" "AUTHOR 

static dev_t		 template_dev_number;
static struct cdev	*driver_object;
static struct class	*template_class;
static struct device	*template_dev;
static wait_queue_head_t wq_read, wq_write;

static atomic_t bytes_that_can_be_written = ATOMIC_INIT(20); /*todo*/
static atomic_t bytes_available           = ATOMIC_INIT(0);  /*todo*/
#define READ_POSSIBLE  (atomic_read(&bytes_available)!=0)
#define WRITE_POSSIBLE (atomic_read(&bytes_that_can_be_written)!=0)

static char kernelmem[MEMBUF];

static int driver_open(struct inode *device_file, struct file *instance){
	dev_info(template_dev, "driver_open called\n");
	return 0;
}

static int driver_close(struct inode *device_file, struct file *instance){
	dev_info(template_dev, "driver_close called\n");
	return 0;
}

static ssize_t driver_read(struct file *instance, char __user *buffer,
	size_t max_bytes_to_read, loff_t *offset){
	
	size_t not_copied, to_copy;

	/*todo: fill kernelmem with any data & edit bytes_available */

	if(!READ_POSSIBLE && (instance->f_flags&O_NONBLOCK))
		return -EAGAIN; /*Nonblocking-mode & no data available*/
	if(wait_event_interruptible(wq_read, READ_POSSIBLE))
		return -ERESTARTSYS; /*Signal interrupt sleeping*/
	to_copy = min((size_t)atomic_read(&bytes_available), max_bytes_to_read);
	not_copied = copy_to_user(buffer, kernelmem, to_copy);
	atomic_sub(to_copy-not_copied, &bytes_available);
	*offset += to_copy-not_copied;
	return to_copy-not_copied;
}

static ssize_t driver_write(struct file *instance, const char __user *buffer,
	size_t max_bytes_to_write, loff_t *offset){
	
	size_t not_copied, to_copy;

	if(!WRITE_POSSIBLE && (instance->f_flags&O_NONBLOCK))
		return -EAGAIN; /*Nonblocking-mode & not ready to write data*/
	if(wait_event_interruptible(wq_write, WRITE_POSSIBLE))
		return -ERESTARTSYS; /*Signal interrupt stopps sleeping*/
	to_copy = min((size_t)atomic_read(&bytes_that_can_be_written),
		 max_bytes_to_write);
	not_copied = copy_from_user(kernelmem, buffer, to_copy);

	/*todo write kernelmem to hardware*/
	dev_info(template_dev,"Received:%d characters from the user\n",max_bytes_to_write);

/*##### MODIFY THIS #####*/
/*	atomic_sub(to_copy-not_copied, &bytes_that_can_be_written); */
	atomic_set(&bytes_available, strlen(kernelmem)+1);
	wake_up_interruptible(&wq_read);
/*######################*/
	
	*offset += to_copy-not_copied;
	return to_copy-not_copied;
}

static struct file_operations fops={
	.owner   = THIS_MODULE,
	.read    = driver_read,
	.write	 = driver_write,
	.open    = driver_open,
	.release = driver_close,
};

static int __init template_init(void){
	init_waitqueue_head(&wq_read);
	init_waitqueue_head(&wq_write);
	if(alloc_chrdev_region(&template_dev_number,0,1,MODULNAME)<0)
		return -EIO;
	driver_object = cdev_alloc();
	if(driver_object==NULL)
		goto free_device_number;
	driver_object->owner = THIS_MODULE;
	driver_object->ops = &fops;
	if(cdev_add(driver_object,template_dev_number,1))
		goto free_cdev;
	template_class = class_create(THIS_MODULE,MODULNAME);
	if(IS_ERR(template_class)){
		pr_err("template: no udev support\n");	
		goto free_cdev;
	}
	template_dev = device_create(template_class, NULL, template_dev_number,
		NULL, "%s",MODULNAME);
	if(IS_ERR(template_dev)){
		pr_err("template: device_create() failed\n");
		goto free_class;
	}
	dev_info(template_dev, "%s\n", VERSION);
	return 0;
free_class:
	class_destroy(template_class);
free_cdev:
	kobject_put(&driver_object->kobj);
free_device_number:
	unregister_chrdev_region(template_dev_number,1);
	return -EIO;
}

static void __exit template_exit(void){
	dev_info(template_dev, "exit\n");
	device_destroy(template_class, template_dev_number);
	class_destroy(template_class);
	cdev_del(driver_object);
	unregister_chrdev_region(template_dev_number,1);
	return;
}

module_init(template_init);
module_exit(template_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION("template for further drivers");
MODULE_VERSION(VERSION_NR);
