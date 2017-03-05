#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
/*#include <linux/init.h>*/
#include <asm/uaccess.h> /* copy_to_user() */
/*#include <linux/kernel.h> */

#define MODULNAME "template"

static char str[] = "Hello World\n";

static dev_t		 template_dev_number;
static struct cdev	*driver_object;
static struct class	*template_class;
static struct device	*template_dev;

static int driver_open( struct inode *device_file, struct file *entity){
	dev_info(template_dev, "driver_open called\n");
	return 0;
}

static int driver_close( struct inode *device_file, struct file *entity){
	dev_info(template_dev, "driver_close called\n");
	return 0;
}

static ssize_t driver_read(struct file *entity, char __user *user,
	size_t count, loff_t *offset){
	unsigned long not_copied, to_copy;

	to_copy = min(count, strlen(str)+1);
	not_copied = copy_to_user(user, str, to_copy);
	*offset += to_copy-not_copied;
	return to_copy-not_copied;
}

static struct file_operations fops={
	.owner   = THIS_MODULE,
	.read    = driver_read,
	.open    = driver_open,
	.release = driver_close,
};

static int __init template_init(void){
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
	device_destroy(template_class, template_dev_number);
	class_destroy(template_class);
	cdev_del(driver_object);
	unregister_chrdev_region(template_dev_number,1);
	return;
}

module_init(template_init);
module_exit(template_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("linkjumper");
MODULE_DESCRIPTION("template for further drivers");
MODULE_VERSION("V0.0");
