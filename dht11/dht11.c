#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include "dht11.h"

#define GPIOPIN2	2
#define DEV_NAME	"dht11" //temp- and humidity-sensor
#define MEMBUF		20
#define HIGH		1
#define LOW			0
#define MAXCYCLES	2000

static dev_t ioctl_dev_number;
static struct cdev *driver_object;
static struct class *ioctl_class;
static struct device *ioctl_dev;

static int countcycles(int gpionr, unsigned int level){
	int ret=0;

	while( (__gpio_get_value(gpionr)) == level){
		if(ret++ >= MAXCYCLES)
			return -1;
	}
	return ret;
}

static int read_data(int gpionr, char *data){
	int err;
	char name[MEMBUF];
	int i;
	int cycles[80];
	int high, low;

	data[2]= 'p';

	snprintf(name, sizeof(name), "rpi-gpio-%d",gpionr);
	err = gpio_request(gpionr, name);
	if(err){
		printk("gpio_request failed %d\n",err);
		gpio_free(gpionr);
		return -1;
	}

	/* output high for 250ms */
	err = gpio_direction_output(gpionr, HIGH);
	if(err)
		goto gpio_dir_error;
	msleep(250);

	/* output low for 20ms */
	err = gpio_direction_output(gpionr, LOW);
	if(err)
		goto gpio_dir_error;
	msleep(20);

	/* output high for 40us */
	err = gpio_direction_output(gpionr, HIGH);
	if(err)
		goto gpio_dir_error;
	usleep_range(39, 41);

	/*set as input */
	err = gpio_direction_input(gpionr);
		if(err)
			goto gpio_dir_error;
//pullup ?
	usleep_range(1, 2);

	countcycles(gpionr, 0);
	countcycles(gpionr, 1);

	for(i=0;i<80;i+=2){
		cycles[i]   = countcycles(gpionr, 0);
		cycles[i+1] = countcycles(gpionr, 1);
	}

	for(i=0;i<40;++i){
		low  = cycles[i*2];
		high = cycles[i*2+1];

		data[i/8] <<= 1;
		if(high>low)
			data[i/8] |= 1;
	}

//pull down?
	printk("data %d successfull read\n", gpionr);
	gpio_free(gpionr);
	return 0;

gpio_dir_error:
		printk("gpio_direction_input/output failed %d\n",err);
		gpio_free(gpionr);
		return -1;

}

static long dht11_ioctl(struct file *instance, unsigned int cmd, unsigned long arg){
	int not_copied;
	struct dht11 * dht = (struct dht11 *)arg;
	char data[5]; // temp: data[2]; humidity: data[5] -> stored as 2 Byte float

	dev_info(ioctl_dev, "ioctl called 0x%4.4x %p\n",cmd, (void *)arg);

	switch(cmd){
		case IOCTL_TEST:
			not_copied = copy_to_user((void*)dht->str, "Hello world!",12);
			break;
		case IOCTL_TEMP:
			read_data(GPIOPIN2, data);
			not_copied = copy_to_user((void*)&dht->value, &data[2], sizeof(char));
			break;
		case IOCTL_HUMI:
			read_data(GPIOPIN2, data);
			not_copied = copy_to_user((void*)&dht->value, &data[5], sizeof(char));
			break;
		default:
			printk("unknown IOCTL 0x%x\n",cmd);
			return -EINVAL;
	}

	return 0;
}

static int dht11_open(struct inode *device_file, struct file *instance){
        dev_info(ioctl_dev, "ioctl_open\n");
        return 0;
}

static int dht11_close(struct inode *device_file, struct file *instance){
        dev_info(ioctl_dev, "ioctl_close\n");
        return 0;
}

static struct file_operations ioctl_fops = {
	.open 			= dht11_open,
	.release 		= dht11_close,
	.owner			= THIS_MODULE,
	.unlocked_ioctl = dht11_ioctl,
};

static int __init dht11_init( void )
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

static void __exit dht11_exit( void )
{
	/* Loeschen des Sysfs-Eintrags und damit der Geraetedatei */
	device_destroy( ioctl_class, ioctl_dev_number );
	class_destroy( ioctl_class );
	/* Abmelden des Treibers */
	cdev_del( driver_object );
	unregister_chrdev_region( ioctl_dev_number, 1 );
	return;
}

module_init( dht11_init );
module_exit( dht11_exit );
MODULE_LICENSE("GPL");
