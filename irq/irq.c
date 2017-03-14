#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>  /*copy_to_user()*/
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/wait.h>

#define MEMBUF		20
#define GPIO_NUM	17

#define MODULNAME	"gpioirq17"
#define VERSION_NR	"V0.0"
#define RELEASE_DATE	"2017-03-14"
#define AUTHOR		"linkjumper"
#define FILENAME	"irq.c"
#define VERSION 	"Id:"FILENAME" "VERSION_NR" "RELEASE_DATE" "AUTHOR 

static dev_t		 gpio_dev_number;
static struct cdev	*driver_object;
static struct class	*gpio_class;
static struct device	*gpio_dev;
static int 		 rpi_irq_17;
static char		*devname = "gpio_irq";
static wait_queue_head_t sleeping_for_ir;
static int 		 interrupt_arrived;


static irqreturn_t rpi_gpio_isr(int p_irq, void *p_data){
	printk("rpi_gpio_isr( %d, %p )\n", p_irq, p_data);
	interrupt_arrived += 1;
	wake_up(&sleeping_for_ir);
	return IRQ_HANDLED;
}

static int config_gpio(int p_gpionr){
	int _err, _rpi_irq;
	char _name[MEMBUF];

	snprintf(_name, sizeof(_name), "rpi-gpio-%d",p_gpionr);
	_err = gpio_request(p_gpionr, _name);
	if(_err){
		printk("gpio_request failed %d\n",_err);
		return -1;
	}
	_err = gpio_direction_input(p_gpionr);
	if(_err){
		printk("gpio_direction_input failed %d\n",_err);
		gpio_free(p_gpionr);
		return -1;
	}
	_rpi_irq = gpio_to_irq(p_gpionr);
	printk("gpio_to_irq returned %d\n",_rpi_irq);
	if(_rpi_irq<0){
		printk("gpio_to_irq failed %d\n",_rpi_irq);
		gpio_free(p_gpionr);
		return -1;	
	}
	_err = request_irq(_rpi_irq, rpi_gpio_isr,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		devname, driver_object);
	printk("driver_object: %p\n", driver_object);
	if(_err){
		printk("request_irq failed with %d\n", _err);
		gpio_free(p_gpionr);
		return -1;
	}
	printk("gpio %d successfull configured\n", p_gpionr);
	return _rpi_irq;
}

static int driver_open(struct inode *device_file, struct file *instance){
	return 0;
}

static int driver_close(struct inode *device_file, struct file *instance){
	return 0;
}

static ssize_t driver_read(struct file *instance, char __user *user,
	size_t count, loff_t *offset){
	
	size_t not_copied, to_copy;

	interrupt_arrived = 0;
	wait_event_interruptible(sleeping_for_ir, interrupt_arrived);	
	to_copy = min(count, sizeof(interrupt_arrived));
	not_copied = copy_to_user(user, &interrupt_arrived, to_copy);
	return to_copy-not_copied;
}

static struct file_operations fops = {
	.owner   = THIS_MODULE,
	.read    = driver_read,
	.open    = driver_open,
	.release = driver_close
};

static int __init mod_init(void){
	init_waitqueue_head(&sleeping_for_ir);
	if(alloc_chrdev_region(&gpio_dev_number,0,1,MODULNAME)<0)
		return -EIO;
	driver_object = cdev_alloc();
	if(driver_object==NULL)
		goto free_device_number;
	driver_object->owner = THIS_MODULE;
	driver_object->ops = &fops;
	if(cdev_add(driver_object,gpio_dev_number,1))
		goto free_cdev;
	gpio_class = class_create(THIS_MODULE,MODULNAME);
	if(IS_ERR(gpio_class)){
		pr_err("%s: no udev support\n", MODULNAME);	
		goto free_cdev;
	}
	gpio_dev = device_create(gpio_class, NULL, gpio_dev_number,
		NULL, "%s",MODULNAME);
	if(IS_ERR(gpio_dev)){
		pr_err("%s: device_create() failed\n",MODULNAME);
		goto free_class;
	}
	rpi_irq_17 = config_gpio(GPIO_NUM);
	if(rpi_irq_17<0){
		pr_err("%s: config_gpio() failed\n",MODULNAME);
		goto free_device;
	}
	dev_info(gpio_dev, "mod_init: %s\n", VERSION);
	return 0;
free_device:
	device_destroy(gpio_class, gpio_dev_number);
free_class:
	class_destroy(gpio_class);
free_cdev:
	cdev_del(driver_object);
	/*kobject_put(&driver_object->kobj);*/
free_device_number:
	unregister_chrdev_region(gpio_dev_number,1);
	return -EIO;
}

static void __exit mod_exit(void){
	dev_info(gpio_dev, "mod_exit");
	device_destroy(gpio_class, gpio_dev_number);
	class_destroy(gpio_class);
	cdev_del(driver_object);
	unregister_chrdev_region(gpio_dev_number,1);
	free_irq(rpi_irq_17, driver_object);
	gpio_free(17);
	return;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION("rpi gpio example");
MODULE_VERSION(VERSION_NR);
