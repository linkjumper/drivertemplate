#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

static int drivertest_probe(struct platform_device *dev)
{
	printk(KERN_ALERT "Probe device: %s\n", dev->name);
	return 0;
}

static int drivertest_remove(struct platform_device *dev)
{
	return 0;
}

static void drivertest_device_release(struct device *dev)
{
}

static struct platform_driver drivertest_driver = {
	.driver = {
		.name = "drivertest",
		.owner = THIS_MODULE,
	},
	.probe = drivertest_probe,
	.remove = drivertest_remove,
};

static struct platform_device drivertest_device = {
	.name = "drivertest",
	.id = 0,
	.dev = {
		.release = drivertest_device_release,
	},
};

static int __init drivertest_init(void)
{
	printk(KERN_ALERT "Driver test init\n");
	platform_driver_register(&drivertest_driver);
	platform_device_register(&drivertest_device);
	return 0;
}

static void __exit drivertest_exit(void)
{
	printk(KERN_ALERT "Driver test exit\n");
	platform_device_unregister(&drivertest_device);
	platform_driver_unregister(&drivertest_driver);
}

module_init(drivertest_init);
module_exit(drivertest_exit);
MODULE_LICENSE("GPL");
