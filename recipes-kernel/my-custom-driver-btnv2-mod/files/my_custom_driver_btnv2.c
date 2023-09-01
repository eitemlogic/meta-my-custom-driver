#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>

struct my_btn_data {
	struct gpio_desc *btn;
	unsigned int irq_number;
};

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

	struct my_btn_data *btn_data;
	
	btn_data = devm_kzalloc(dev, sizeof(struct my_btn_data), GFP_KERNEL);
	if (!btn_data) {
		printk("Unable to allocate memory for btn struct\n");
		return -ENOMEM;
	}

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
	btn_data->btn = devm_fwnode_gpiod_get(dev, fwnode_btn0, NULL, GPIOD_IN, label);
	if(IS_ERR(btn_data->btn)) {
		printk("Error, could not set up GPIO\n");
		return -EIO;
	}
	printk("Button set up as GPIO device\n");

	// Get valid IRQ number
	btn_data->irq_number = gpiod_to_irq(btn_data->btn);
	printk("IRQ number: %d\n", btn_data->irq_number);

	// Set up IRQ
	err = devm_request_irq(dev, btn_data->irq_number,
						   (irq_handler_t) btn_irq_handler,
					 	   IRQF_TRIGGER_FALLING,
					       "btn0_irq", NULL);
	if (err) {
		printk("Error, could not request IRQ\n");
		return -EIO;
	}

	// Set debounce interval
	gpiod_set_debounce(btn_data->btn, db * 1000);

	platform_set_drvdata(pdev, btn_data);

    return 0;
}

static int my_module_remove(struct platform_device *pdev)
{
	printk("Running device remove\n");

	struct my_btn_data *btn_data = platform_get_drvdata(pdev);
	printk("IRQ number: %d\n", btn_data->irq_number);

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

module_platform_driver(my_device_driver);
MODULE_LICENSE("GPL");