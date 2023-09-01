#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>

static struct gpio_desc *led0 = NULL;
static int led0_state = 0;
static struct kobject *led0_kobj;

static ssize_t my_module_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    // Write current LED state to user buffer
	return sprintf(buf, "%d\n", led0_state);
}

static ssize_t my_module_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    // Read input and update LED state
	sscanf(buf, "%du", &led0_state);
	if (led0_state == 1 || led0_state == 0) {
		gpiod_set_value(led0, led0_state);
	}
	return count;
}

static struct kobj_attribute led_state_attr = __ATTR(led_state, 0660, my_module_show, my_module_store);

static int my_module_probe(struct platform_device *pdev)
{
	printk("Running device probe\n");

	int err;
	struct device *dev = &pdev->dev;
	int db;
    const char *label;

	// Hardcoded fetch hps_led0
	struct fwnode_handle *fwnode_btn0 = device_get_named_child_node(dev, "hps0");

	// Fetch label
	err = fwnode_property_read_string(fwnode_btn0, "label", &label); 
    if (err) {
        printk("Failed to read label property\n");
        return -ENODATA;
    }
    printk("Label is: %s\n", label);

    // Set up LED as GPIO output device, sets led HIGH
	led0 = devm_fwnode_gpiod_get(dev, fwnode_btn0, NULL, GPIOD_OUT_HIGH, label);
	if (IS_ERR(led0)) {
		printk("Error, could not set up GPIO\n");
		return -EIO;
	}
	printk("LED set up as GPIO device\n");

    // Store current state (HIGH)
    led0_state = 1;

    // Create sysfs entry
	led0_kobj = kobject_create_and_add("my_led_sysfs", NULL);
	if(!led0_kobj) {
		printk("Failed to create sysfs entry\n");
		return -ENOMEM;
    }
    // Add attribute
	err = sysfs_create_file(led0_kobj, &led_state_attr.attr);
	if (err) {
		printk("Failed to create sysfs attribute\n");
        // Cleanup sysfs entry
        kobject_put(led0_kobj);
        return -ENOMEM;
	}

	return 0;
}

static int my_module_remove(struct platform_device *pdev)
{
	printk("Running device remove\n");
    // Turn off LED
    gpiod_set_value(led0, 0);
    // Cleanup sysfs entry
	sysfs_remove_file(led0_kobj, &led_state_attr.attr);
    kobject_put(led0_kobj);
	return 0;
}

static struct of_device_id my_driver_of_match[] = {
	{ .compatible = "my-gpio-led", },
	{ },
};
MODULE_DEVICE_TABLE(of, my_driver_of_match);

static struct platform_driver my_device_driver = {
	.probe = my_module_probe,
	.remove = my_module_remove,
	.driver = {
		.name = "my_device_driver",
		.of_match_table = my_driver_of_match,
	}
};

static int __init my_module_init(void)
{
	printk("Hello World!\n");
	platform_driver_register(&my_device_driver);
	return 0;
}

static void __exit my_module_exit(void)
{
	printk("Goodbye Cruel World!\n");
	platform_driver_unregister(&my_device_driver);
}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");