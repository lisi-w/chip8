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
 * insmod fpga_ram.ko
 *
 * Check code style with
 * checkpatch.pl --file --no-tree fpga_ram.c
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
#include "fpga_ram.h"

#define DRIVER_NAME "chip8"
#define VIRT_OFF(x,a) ((x)+(a))
#define VIRT_OFF_LONG(x, a) ((x)+(4*(a)))

/*
 * Information about our device
 */
struct fpga_ram_dev {
	struct resource res; /* Resource: our registers */
	void __iomem *virtbase; /* Where registers can be accessed in memory */
    fpga_ram_arg_t ram_args;
} dev;

/*
 * Write segments of a single digit
 * Assumes digit is in range and the device information has been set up
 */
static void write_ram(fpga_ram_arg_t *ram_args) {
    iowrite8(ram_args->data, VIRT_OFF(dev.virtbase, ram_args->address));
    dev.ram_args = *ram_args;
}

/*
 * Write segments of a single digit
 * Assumes digit is in range and the device information has been set up
 */
static void write_ram_long(fpga_ram_arg_t *ram_args) {
    iowrite32(ram_args->data, VIRT_OFF_LONG(dev.virtbase, ram_args->address));
    dev.ram_args = *ram_args;
}


static void read_ram(fpga_ram_arg_t *ram_args)
{
    ram_args->data = ioread8(VIRT_OFF(dev.virtbase, ram_args->address));
}

static void read_ram_long(fpga_ram_arg_t *ram_args)
{
    ram_args->data = ioread32(VIRT_OFF_LONG(dev.virtbase, ram_args->address));
}

/*
 * Handle ioctl() calls from userspace:
 * Read or write the segments on single digits.
 * Note extensive error checking of arguments
 */
static long fpga_ram_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	fpga_ram_arg_t vla;

	switch (cmd) {
	case FPGA_RAM_WRITE:
		if (copy_from_user(&vla, (fpga_ram_arg_t *) arg,
				   sizeof(fpga_ram_arg_t)))
			return -EACCES;
		write_ram(&vla);
		break;

	case FPGA_RAM_WRITE_L:
		if (copy_from_user(&vla, (fpga_ram_arg_t *) arg,
				   sizeof(fpga_ram_arg_t)))
			return -EACCES;
		write_ram_long(&vla);
		break;
	case FPGA_RAM_READ:
        if (copy_from_user(&vla,(fpga_ram_arg_t *) arg, 
                           sizeof (fpga_ram_arg_t)))
                 return -EACCES;
        read_ram(&vla);
		if (copy_to_user((fpga_ram_arg_t *) arg, &vla,
				 sizeof(fpga_ram_arg_t)))
			return -EACCES;
		break;

	case FPGA_RAM_READ_L:
        if (copy_from_user(&vla,(fpga_ram_arg_t *) arg, 
                           sizeof (fpga_ram_arg_t)))
                 return -EACCES;
        read_ram_long(&vla);
		if (copy_to_user((fpga_ram_arg_t *) arg, &vla,
				 sizeof(fpga_ram_arg_t)))
			return -EACCES;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* The operations our device knows how to do */
static const struct file_operations fpga_ram_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = fpga_ram_ioctl,
};

/* Information about our device for the "misc" framework -- like a char dev */
static struct miscdevice fpga_ram_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DRIVER_NAME,
	.fops		= &fpga_ram_fops,
};

/*
 * Initialization code: get resources (registers) and display
 * a welcome message
 */
static int __init fpga_ram_probe(struct platform_device *pdev)
{
	int ret;

	/* Register ourselves as a misc device: creates /dev/chip8 */
	ret = misc_register(&fpga_ram_misc_device);

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
	misc_deregister(&fpga_ram_misc_device);
	return ret;
}

/* Clean-up code: release resources */
static int fpga_ram_remove(struct platform_device *pdev)
{
	iounmap(dev.virtbase);
	release_mem_region(dev.res.start, resource_size(&dev.res));
	misc_deregister(&fpga_ram_misc_device);
	return 0;
}

/* Which "compatible" string(s) to search for in the Device Tree */
#ifdef CONFIG_OF
static const struct of_device_id fpga_ram_of_match[] = {
	{ .compatible = "csee4840,chip8-1.0" },
	{},
};
MODULE_DEVICE_TABLE(of, fpga_ram_of_match);
#endif

/* Information for registering ourselves as a "platform" driver */
static struct platform_driver fpga_ram_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(fpga_ram_of_match),
	},
	.remove	= __exit_p(fpga_ram_remove),
};

/* Called when the module is loaded: set things up */
static int __init fpga_ram_init(void)
{
	pr_info(DRIVER_NAME ": init\n");
	return platform_driver_probe(&fpga_ram_driver, fpga_ram_probe);
}

/* Calball when the module is unloaded: release resources */
static void __exit fpga_ram_exit(void)
{
	platform_driver_unregister(&fpga_ram_driver);
	pr_info(DRIVER_NAME ": exit\n");
}

module_init(fpga_ram_init);
module_exit(fpga_ram_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Elysia Witham, Columbia University");
MODULE_DESCRIPTION("FPGA RAM driver");
