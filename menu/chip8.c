/* * Device driver for FPGA memory
 *
 * A Platform device implemented using the misc subsystem
 *
 * Stephen A. Edwards
 * Columbia University
 *
 * References:
 * Linux source: Documentation/driver-model/platform.txt
 *               drivers/misc/arm-charlcd.c
 * http://www.linuxforu.com/tag/linux-device-drivers/
 * http://free-electrons.com/docs/
 *
 * "make" to build
 * insmod chip8.ko
 *
 */

/* Adapted for use with CHIP8 Final Project
 * Elysia Witham
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "chip8.h"

#define DRIVER_NAME "chip8"
#define VIRT_OFF(x,a) ((x)+(a))
#define VIRT_OFF_LONG(x, a) ((x)+(4*(a)))

/*
 * Information about our device
 */
struct chip8_dev {
	struct resource res; /* Resource: our registers */
	void __iomem *virtbase; /* Where registers can be accessed in memory */
} dev;

/*
 * Write segments of a single digit
 * Assumes digit is in range and the device information has been set up
 */
static void write_instr(unsigned int addr, unsigned int instr) {
    iowrite8(instr, VIRT_OFF(dev.virtbase, addr));
}

/*
 * Handle ioctl() calls from userspace:
 * Read or write the segments on single digits.
 * Note extensive error checking of arguments
 */
static long chip8_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	opcode op;

	switch (cmd) {
	case CHIP8_INSTR_WRITE:
		if (copy_from_user(&op, (opcode *) arg,
				   sizeof(opcode)))
			return -EACCES;
		write_instr(op.addr, op.data);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/* The operations our device knows how to do */
static const struct file_operations chip8_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = chip8_ioctl,
};

/* Information about our device for the "misc" framework -- like a char dev */
static struct miscdevice chip8_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DRIVER_NAME,
	.fops		= &chip8_fops,
};

/*
 * Initialization code: get resources (registers) and display
 * a welcome message
 */
static int __init chip8_probe(struct platform_device *pdev)
{
	int ret;

	/* Register ourselves as a misc device: creates /dev/chip8 */
	ret = misc_register(&chip8_misc_device);

	/* Get the address of our registers from the device tree */
	ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res);
	if (ret) {
		ret = -ENOENT;
		goto out_deregister;
	}

	/* Make sure we can use these registers */
	if (request_mem_region(dev.res.start, resource_size(&dev.res),
			       DRIVER_NAME) == NULL) {
		ret = -EBUSY;
		goto out_deregister;
	}

	/* Arrange access to our registers */
	dev.virtbase = of_iomap(pdev->dev.of_node, 0);
	if (dev.virtbase == NULL) {
		ret = -ENOMEM;
		goto out_release_mem_region;
	}
        
	/* Set an initial mem val?  */

	return 0;

out_release_mem_region:
	release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
	misc_deregister(&chip8_misc_device);
	return ret;
}

/* Clean-up code: release resources */
static int chip8_remove(struct platform_device *pdev)
{
	iounmap(dev.virtbase);
	release_mem_region(dev.res.start, resource_size(&dev.res));
	misc_deregister(&chip8_misc_device);
	return 0;
}

/* Which "compatible" string(s) to search for in the Device Tree */
#ifdef CONFIG_OF
static const struct of_device_id chip8_of_match[] = {
	{ .compatible = "csee4840,chip8-1.0" },
	{},
};
MODULE_DEVICE_TABLE(of, chip8_of_match);
#endif

/* Information for registering ourselves as a "platform" driver */
static struct platform_driver chip8_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(chip8_of_match),
	},
	.remove	= __exit_p(chip8_remove),
};

/* Called when the module is loaded: set things up */
static int __init chip8_init(void)
{
	pr_info(DRIVER_NAME ": init\n");
	return platform_driver_probe(&chip8_driver, chip8_probe);
}

/* Calball when the module is unloaded: release resources */
static void __exit chip8_exit(void)
{
	platform_driver_unregister(&chip8_driver);
	pr_info(DRIVER_NAME ": exit\n");
}

module_init(chip8_init);
module_exit(chip8_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Elysia Witham, Columbia University");
MODULE_DESCRIPTION("CHIP8 driver");
