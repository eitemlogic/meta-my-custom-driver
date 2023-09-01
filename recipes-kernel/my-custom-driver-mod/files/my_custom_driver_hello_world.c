#include <linux/module.h>

static int __init my_module_init(void)
{
	printk("Hello World!\n");
	return 0;
}

static void __exit my_module_exit(void)
{
	printk("Goodbye Cruel World!\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");