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
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static DECLARE_COMPLETION( dev_obj_is_free );
static int frequenz; /* statevariable of the device */

static void mydevice_release( struct device *dev )
{
	complete( &dev_obj_is_free );
}

struct platform_driver mydriver = {
	.driver = {
		.name = "MyDevDrv",
		.owner= THIS_MODULE,
	},
};

struct platform_device mydevice = {
	.name  = "MyDevice",
	.id    = 0,
	.dev = {
		.release = mydevice_release,
	}
};

static ssize_t read_freq( struct device *dev,
	struct device_attribute *attr, char *buf )
{
	snprintf(buf, 256, "frequency: %d\n", frequenz ); 
	return strlen(buf)+1;
}

static ssize_t write_freq( struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count )
{
	int result;

	result = kstrtoint( buf, 0, &frequenz );
	if (result < 0)
		return 0;
	return strlen(buf)+1;
}

static DEVICE_ATTR( freq ,S_IRUGO|S_IWUSR, read_freq, write_freq );

static int __init drv_init(void)
{
	if (platform_driver_register(&mydriver)<0)
		return -EIO;
	if (platform_device_register( &mydevice )) /* register the device */
		goto ex_driverunreg;
	mydevice.dev.driver = &mydriver.driver;    /* now tie them together */
	device_lock( &mydevice.dev );
	if (device_bind_driver( &mydevice.dev )) {/* links the drvr to the dev */
		device_unlock( &mydevice.dev );
		goto ex_platdevunreg;
	}
	device_unlock( &mydevice.dev );
	if (device_create_file( &mydevice.dev, &dev_attr_freq ))
		goto ex_release;
	return 0;
ex_release:
	device_release_driver( &mydevice.dev );
ex_platdevunreg:
	printk("device_bind_driver failed...\n");
	platform_device_unregister( &mydevice );
ex_driverunreg:
	platform_driver_unregister(&mydriver);
	return -EIO;
}

static void __exit drv_exit(void)
{
	device_remove_file( &mydevice.dev, &dev_attr_freq );
	device_release_driver( &mydevice.dev );
	platform_device_unregister( &mydevice );
	platform_driver_unregister(&mydriver);
	wait_for_completion( &dev_obj_is_free );
}

module_init( drv_init );
module_exit( drv_exit );
MODULE_LICENSE("GPL");

