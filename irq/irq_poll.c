/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>

#define IRQDEV_NAME                     "irq"
#define IRQDEV_MAJOR                    101
#define IRQDEV_MINOR                    0

#define IRQ_IOCTL_MODE                  (0x0U)
#define WAIT_FOR_IRQ_TIMEOUT_SECS       10

struct irq_data1 {
    struct completion  irq_done;
    int                irq_number;
};

static struct irq_data1 bal;

static irqreturn_t irq_handler(int irq, void *dev_id)
{
    complete(&bal.irq_done);
    return IRQ_HANDLED;
}

static int
irqdev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    int status = 0;

    do
    {
        status = wait_for_completion_timeout(
            &bal.irq_done, (WAIT_FOR_IRQ_TIMEOUT_SECS * HZ));

        if (status < 0) {
            printk("irq_syscall - Read Timeout waiting for IRQ\n");
        }

        status = gpio_get_value(23);
    }while(status != 1);

	return status;
}

static int irqdev_open(struct inode *inode, struct file *filp)
{
    reinit_completion(&bal.irq_done);
    return 0;
}

static int irqdev_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long
irqdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static const struct file_operations irqdev_fops = {
    .owner =    THIS_MODULE,
    .read =        irqdev_read,
    .open =        irqdev_open,
    .unlocked_ioctl = irqdev_ioctl,
    .release =    irqdev_release,
    .llseek =    no_llseek,
};

static int __init irqdev_init(void)
{
    int status = 0;
    
    status = register_chrdev(IRQDEV_MAJOR, IRQDEV_NAME, &irqdev_fops);
    if (status < 0){
         printk("irq_syscall - Can not register irq\n");
         return status;
    }

    //if(gpio_request(23, "rpi-gpio-23")) {
    if(gpio_request_one(23, (GPIOF_IN | GPIOF_OPEN_DRAIN), "rpi-gpio-23")) {
        printk("irq_syscall - Can not allocate GPIO 23\n");
        goto err_reg_drv;
    }

    /* Set GPIO 23 direction */
/*    if(gpio_direction_input(23)) {
        printk("irq_syscall - Can not set GPIO 23 to input!\n");
        gpio_free(23);
        goto err_reg_drv;
    }
*/

    /* Setup the interrupt */
    bal.irq_number = gpio_to_irq(23);
    if (bal.irq_number < 0) {
        printk("irq_syscall - GPIO mapped as IRQ pin can't be used as IRQ!\n");
        gpio_free(23);
        status = bal.irq_number;
        goto err_reg_drv;
    }

    status = request_threaded_irq(bal.irq_number, irq_handler,
        NULL, IRQF_TRIGGER_RISING, "irq", NULL);
    if (status != 0) {
        gpio_free(23);
        printk("irq_syscall - Cant request_threaded_irq on IRQ Pin!\n");
        goto err_reg_drv;
    }

    init_completion(&bal.irq_done);

    printk("irq_syscall - Inserted Kernel IRQ successful\n");
    return 0;

err_reg_drv:
    unregister_chrdev(IRQDEV_MAJOR, IRQDEV_NAME);
    
    return status;
}
module_init(irqdev_init);

static void __exit irqdev_exit(void)
{
    printk("irq_syscall - Removed Kernel IRQ successful\n");
    free_irq(bal.irq_number, NULL);
    gpio_free(23);
    unregister_chrdev(IRQDEV_MAJOR, IRQDEV_NAME);
}
module_exit(irqdev_exit);

MODULE_AUTHOR("NXP India");
MODULE_DESCRIPTION("NXP RdLib IRQ Kernel Module");
MODULE_LICENSE("GPL");
