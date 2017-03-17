/************************************************************************/
/* Quellcode zum Buch                                                   */
/*                     Linux Treiber entwickeln                         */
/* (4. Auflage) erschienen im dpunkt.verlag                             */
/* Copyright (c) 2004-2015 Juergen Quade und Eva-Katharina Kunst        */
/*                                                                      */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         */
/* GNU General Public License for more details.                         */
/*                                                                      */
/************************************************************************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>

static dev_t my_dev_number;
static struct cdev *driver_object;
static struct class *book_class;
static struct platform_device_id pdi;

static struct file_operations fops;
static DECLARE_COMPLETION( dev_obj_is_free );

static int mydevice_probe_device( struct platform_device *dev )
{
	pr_info("mydevice_probe_device( %p )\n", dev); 
	pr_info("pd->id: %d\n", dev->id );
	return 0;
}

static int mydevice_remove_device( struct platform_device *dev )
{
	pr_info("mydevice_remove_device( %p )\n", dev); 
	pr_info("pd->id: %d\n", dev->id );

	return 0;
}

static void mydevice_release( struct device *dev )
{
	pr_info("mydevice_release()\n");
	complete( &dev_obj_is_free );
}

struct platform_device mydevice = {
	.name  = "my_dev", /* driver identification */
	.id    = 0,
	.dev = {
		.release = mydevice_release,
	}
};

static struct platform_driver mydriver = {
	.driver = {
		.name = "my_dev",
	},
	.probe = mydevice_probe_device,
	.remove = mydevice_remove_device,
	
};

static int __init mod_init(void)
{
	pr_info("mod_init()\n");
	strcpy( pdi.name, "my_dev" );
	mydriver.id_table = &pdi;
	if (platform_driver_register(&mydriver)!=0) {
		pr_err("driver_register failed\n");
		return -EIO;
	}
	if (alloc_chrdev_region(&my_dev_number,0,1,"book")<0)
		return -EIO;
	driver_object = cdev_alloc();
	if (driver_object==NULL)
		goto free_device_number;
	driver_object->owner = THIS_MODULE;
	driver_object->ops = &fops;
	if (cdev_add(driver_object,my_dev_number,1))
		goto free_cdev;
	book_class = class_create( THIS_MODULE, "book" );
	if (IS_ERR(book_class)) {
		printk("book: no udev support.\n");
		goto free_cdev;
	}
	mydevice.dev.devt = my_dev_number;
	platform_device_register( &mydevice );
	return 0;

free_cdev:
	kobject_put( &driver_object->kobj );
free_device_number:
	unregister_chrdev_region( my_dev_number, 3 );
	return -EIO;
}

static void __exit mod_exit(void)
{
	pr_info("mod_exit()\n");
	device_release_driver( &mydevice.dev );
	platform_device_unregister( &mydevice );
	class_destroy( book_class );
	cdev_del( driver_object );
	unregister_chrdev_region( my_dev_number, 1 );
	platform_driver_unregister(&mydriver);
	wait_for_completion( &dev_obj_is_free );
}

module_init( mod_init );
module_exit( mod_exit );
MODULE_LICENSE("GPL");
