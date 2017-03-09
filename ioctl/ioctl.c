#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#define IOCTL_GETVALUE 0x0001
#define DEV_NAME	"myioctl"

static dev_t ioctl_dev_number;
static struct cdev *driver_object;
static struct class *ioctl_class;
static struct device *ioctl_dev;

static long driver_ioctl(struct file *instance, unsigned int cmd, unsigned long arg){
	int not_copied;
	dev_info(ioctl_dev, "ioctl called 0x%4.4x %p\n",cmd, (void *)arg);
	switch(cmd){
	case IOCTL_GETVALUE:
		not_copied=copy_to_user((void*)arg, "Hallöchen\n",11);
		break;
	default:
		printk("unknown IOCTL 0x%x\n",cmd);
		return -EINVAL;
	}
	return 0;
}
/*
static int driver_open(struct inode *device_file, struct file *instance){
        dev_info(ioctl_dev, "ioctl_open\n");
        return 0;
}

static int driver_close(struct inode *device_file, struct file *instance){
        dev_info(ioctl_dev, "ioctl_close\n");
        return 0;
}
*/
static struct file_operations ioctl_fops = {
	/*.open = driver_open,
	.release = driver_close,*/
	.owner= THIS_MODULE,
	/*.compat_ioctl=driver_ioctl does not work*/
	.unlocked_ioctl=driver_ioctl
};

static int __init mod_init( void )
{
	if (alloc_chrdev_region(&ioctl_dev_number,0,1,DEV_NAME)<0)
		return -EIO;
	driver_object = cdev_alloc(); /* Anmeldeobjekt reservieren */
	if (driver_object==NULL)
		goto free_device_number;
	driver_object->owner = THIS_MODULE;
	driver_object->ops = &ioctl_fops;
	if (cdev_add(driver_object,ioctl_dev_number,1))
		goto free_cdev;
	/* Eintrag im Sysfs, damit Udev den Geraetedateieintrag erzeugt. */
	ioctl_class = class_create( THIS_MODULE, DEV_NAME );
	if(IS_ERR(ioctl_class)){
		pr_err("ioctl:no udev support\n");
		goto free_cdev;
	}
	ioctl_dev = device_create( ioctl_class, NULL, ioctl_dev_number,
			NULL, "%s", DEV_NAME );
	if(IS_ERR(ioctl_dev)){
		pr_err("ioctl: device create faileed\n");
		goto free_class;
	}
	dev_info(ioctl_dev, "__init called\n");
	return 0;
free_class:
	class_destroy(ioctl_class);
free_cdev:
	kobject_put( &driver_object->kobj );
free_device_number:
	unregister_chrdev_region( ioctl_dev_number, 1 );
	return -EIO;
}

static void __exit mod_exit( void )
{
	/* Loeschen des Sysfs-Eintrags und damit der Geraetedatei */
	device_destroy( ioctl_class, ioctl_dev_number );
	class_destroy( ioctl_class );
	/* Abmelden des Treibers */
	cdev_del( driver_object );
	unregister_chrdev_region( ioctl_dev_number, 1 );
	return;
}

module_init( mod_init );
module_exit( mod_exit );
MODULE_LICENSE("GPL");
