#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("Dual BSD/GPL");

/* module init */
static int initModule(void)
{
	printk(KERN_INFO "Hello module!\n");
	return 0;
}

static void cleanupModule(void)
{
	printk(KERN_INFO "Good-bye module!\n");
}

module_init(initModule);
module_exit(cleanupModule)
