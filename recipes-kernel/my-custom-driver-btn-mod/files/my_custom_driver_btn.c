#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>

static struct gpio_desc *btn0 = NULL;
static unsigned int irq_number;


static irq_handler_t btn_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs) {
	printk("Btn interrupt triggered!\n");
	return (irq_handler_t) IRQ_HANDLED;
}

static int my_module_probe(struct platform_device *pdev)
{
	printk("Running device probe\n");

	int err;
	struct device *dev = &pdev->dev;
	int db;
    const char *label;

	// Hardcoded fetch btn0
	struct fwnode_handle *fwnode_btn0 = device_get_named_child_node(dev, "btn0");

	// Fetch label
	err = fwnode_property_read_string(fwnode_btn0, "label", &label); 
    if (err) {
        printk("Failed to read label property\n");
        return -ENODATA;
    }
    printk("Label is: %s\n", label);
	
	// Fetch debounce interval
	err = fwnode_property_read_u32(fwnode_btn0, "debounce-interval", &db); 
    if (err) {
        printk("Failed to read debounce interval property\n");
        return -ENODATA;
    }
	printk("Debounce interval is: %d\n", db);

	// Set up button as GPIO input device
	btn0 = devm_fwnode_gpiod_get(dev, fwnode_btn0, NULL, GPIOD_IN, label);
	if(IS_ERR(btn0)) {
		printk("Error, could not set up GPIO\n");
		return -EIO;
	}
	printk("Button set up as GPIO device\n");

	// Get valid IRQ number
	irq_number = gpiod_to_irq(btn0);
	printk("IRQ number: %d\n", irq_number);

	// Set up IRQ
	err = devm_request_irq(dev, irq_number,
						   (irq_handler_t) btn_irq_handler,
					 	   IRQF_TRIGGER_FALLING,
					       "btn0_irq", NULL);
	if (err) {
		printk("Error, could not request IRQ\n");
		return -EIO;
	}

	// Set debounce interval
	gpiod_set_debounce(btn0, db * 1000);

    return 0;
}


static int my_module_remove(struct platform_device *pdev)
{
	printk("Running device remove\n");
	return 0;
}

static struct of_device_id my_driver_of_match[] = {
	{ .compatible = "my-gpio-button", },
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