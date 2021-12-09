#include <linux/module.h>	
#include <linux/kernel.h>	
#include <linux/init.h>		
#include <linux/stat.h>		
#include <linux/device.h>	
#include <linux/fs.h>		
#include <linux/err.h>		
#include <linux/semaphore.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/io.h>	
#include <linux/sched.h>

#include "modgpio.h"

#define IO_OFFSET			 	0x0// 0xfd000000 // 0xfec00000 // 
#define __IO_ADDRESS(x)			((x) + IO_OFFSET)
#define IO_ADDRESS(pa)			IOMEM(__IO_ADDRESS(pa))
#define __io_address(n)		((void __iomem *)IO_ADDRESS(n))


#define RPIGPIO_MOD_AUTH 	"Ivanov Vsedolod"
#define RPIGPIO_MOD_DESC 	"OS cource work (GPIO access control for Raspberry Pi)"
#define RPIGPIO_MOD_SDEV 	"RPiGPIO" 	
#define MOD_NAME 			"rpigpio" 	

#define PIN_NULL_PID		1	// invalid pin
#define PIN_UNASSN 			0	// pin available
#define PIN_ARRAY_LEN 		32

struct gpiomod_data {
	int irq;
	int mjr;
	struct class *cls;
	void __iomem *regs;
	spinlock_t lock;
	uint32_t pins[PIN_ARRAY_LEN];
};

static struct gpiomod_data std = {
	.mjr = 0, 
	.cls = NULL, 
	.regs = NULL,
	.pins = {			
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_NULL_PID,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_NULL_PID,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_UNASSN,
		PIN_UNASSN,
	}
};


static int st_open(struct inode*inode, struct file *filp)
{
	return 0;
}

static int st_release(struct inode *inode, struct file *filp)
{
	int i;

	spin_lock(&std.lock);
	for (i = 0; i < PIN_ARRAY_LEN; i++) {
		if (std.pins[i] == current->pid) {
			printk(KERN_DEBUG "[FREE] Pin:%d From:%d\n", i, current->pid);
			std.pins[i] = PIN_UNASSN;
		}
	}
	spin_unlock(&std.lock);
	return 0;
}


static long rpigpio_toggle(int pin, uint8_t *flag)
{
	uint32_t val;

	spin_lock(&std.lock);
	// validate pins
	if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) { 
		spin_unlock(&std.lock);
		return -EFAULT;	// bad request
	} else if (std.pins[pin] != current->pid) {
		spin_unlock(&std.lock);
		return -EACCES;	// permission denied
	}
	spin_unlock(&std.lock);

	val = readl(__io_address(std.regs + GPLEV0));
	*flag = val >> (pin%32);
	*flag &= 0x01;

	printk(KERN_DEBUG "[TOGGLE] Pin:%d From:%.1d To:%.1d\n", pin, *flag, *flag?0:1);
	if (*flag)
		writel(1 << pin, __io_address(std.regs + GPCLR0));	// clear
	else
		writel(1 << pin, __io_address(std.regs + GPSET0));	// set
	return 0;
}

static long st_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int pin;			//used in read, request, free
	unsigned long ret, code;
	uint32_t val;
	uint8_t flag;
	struct gpio_data_write wdata;	// write data
	struct gpio_data_mode  mdata;	// mode data
	
	switch (cmd) {
		case GPIO_READ:
			get_user(pin, (int __user *) arg);

			val = readl(__io_address(std.regs + GPLEV0));
			flag = val >> (pin%32);
			flag &= 0x01;
			printk(KERN_DEBUG "[READ] Pin: %d Val:%d\n", pin, flag);

			put_user(flag, (uint8_t __user *)arg);
			return 0;
	
		case GPIO_WRITE:
			ret = copy_from_user(&wdata, (struct gpio_data_write __user *)arg, sizeof(struct gpio_data_write));
			if (ret != 0) {
				printk(KERN_DEBUG "[WRITE] Error copying data from userspace\n");
				return -EFAULT;
			}

			pin = wdata.pin;
			spin_lock(&std.lock);
			//validate pins
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) {
				spin_unlock(&std.lock);
				return -EFAULT;	// bad request
			} else if (std.pins[pin] != current->pid) {
				spin_unlock(&std.lock);
				return -EACCES;	// pin reserved by another process
			}
			spin_unlock(&std.lock);

			printk(KERN_INFO "[WRITE] Pin: %d Val:%d\n", wdata.pin, wdata.data);
			if (wdata.data)
				writel(1 << wdata.pin, __io_address(std.regs + GPSET0)); // set
			else
				writel(1 << wdata.pin, __io_address(std.regs + GPCLR0)); // clear

			return 0;
	
		case GPIO_REQUEST:
			get_user (pin, (int __user *) arg);

			spin_lock(&std.lock);
			// validate pins
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) {
				spin_unlock(&std.lock);
				return -EFAULT;	// bad request
			} else if (std.pins[pin] != PIN_UNASSN) {
				spin_unlock(&std.lock);
				return -EBUSY;	// pin already reserved
			}
			std.pins[pin] = current->pid;
			spin_unlock(&std.lock);

			printk(KERN_DEBUG "[REQUEST] Pin:%d Assn To:%d\n", pin, current->pid);
			return 0;


		case GPIO_FREE:
			get_user (pin, (int __user *) arg);

			spin_lock(&std.lock);
			//validate pins
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) {
				spin_unlock(&std.lock);
				return -EFAULT;	// bad request
			} else if (std.pins[pin] != current->pid) {
				spin_unlock(&std.lock);
				return -EACCES;	// pin reserved by another process
			}
			std.pins[pin] = PIN_UNASSN;
			spin_unlock(&std.lock);
			printk(KERN_DEBUG "[FREE] Pin:%d From:%d\n", pin, current->pid);
			return 0;

		case GPIO_TOGGLE:
			get_user (pin, (int __user *) arg);

			code = rpigpio_toggle(pin, &flag);
			if (!code)
				put_user(flag?0:1, (uint8_t __user *)arg);

			return code;

		case GPIO_MODE:
			ret = copy_from_user(&mdata, (struct gpio_data_mode __user *)arg, sizeof(struct gpio_data_mode));
			if (ret != 0) {
				printk(KERN_DEBUG "[MODE] Error copying data from userspace\n");
				return -EFAULT;
			}

			spin_lock(&std.lock);
			// validate pin
			if (mdata.pin > PIN_ARRAY_LEN || mdata.pin < 0 || std.pins[mdata.pin] == PIN_NULL_PID) { 
				spin_unlock(&std.lock);
				return -EFAULT;	// bad request
			} else if (std.pins[mdata.pin] != current->pid) {
				spin_unlock(&std.lock);
				return -EACCES;	// permission denied
			}
			spin_unlock(&std.lock);

			//clear the bits (sets to input)
			writel(~(7<<((mdata.pin %10)*3)) & readl(__io_address(std.regs + GPFSEL0 + (0x04)*(mdata.pin /10))), __io_address(std.regs + GPFSEL0 + (0x04)*(mdata.pin/10)));
			if(mdata.data == MODE_INPUT) {
				printk(KERN_DEBUG "[MODE] Pin %d set as Input\n", mdata.pin);
			} else if (mdata.data == MODE_OUTPUT) {
				writel(1<<((mdata.pin % 10)*3) | readl(__io_address(std.regs + GPFSEL0 + (0x04)*(mdata.pin/10))), __io_address(std.regs + GPFSEL0 + (0x04)*(mdata.pin/10)));    // Set pin as output
				printk(KERN_DEBUG "[MODE] Pin %d set as Output\n", mdata.pin);
			} else {
				return -EINVAL;	//Invalid argument
			}
			return 0;
		case GPIO_SET:
		case GPIO_CLR:
			get_user (pin, (int __user *) arg);
			
			spin_lock(&std.lock);
			// validate pins
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) 
			{ 
				spin_unlock(&std.lock);
				return -EFAULT;	// bad request
			} 
			else if (std.pins[pin] != current->pid) 
			{
				spin_unlock(&std.lock);
				return -EACCES;	// permission denied
			}
			spin_unlock(&std.lock);

			if (cmd == GPIO_SET) {
				writel(1 << pin, __io_address(std.regs + GPSET0));	// set
				printk(KERN_INFO "[SET] Pin: %d\n", pin);
			} else {
				writel(1 << pin, __io_address(std.regs + GPCLR0));	// clear
				printk(KERN_INFO "[CLR] Pin: %d\n", pin);
			}

			return 0;
		default:
			return -ENOTTY;	//Error Message: inappropriate IOCTL for device
	}
}

static char *st_devnode(struct device *dev, umode_t *mode)
{
	if (mode) *mode = 0666;
	return NULL;
}


static const struct file_operations gpio_fops = {
	.owner			= THIS_MODULE,
	.open			= st_open,
	.release 		= st_release,
	.unlocked_ioctl = st_ioctl,
};

static irqreturn_t button_int (int irq, void *dev_id)
{
	uint8_t flag;
	printk(KERN_DEBUG "[IRQ] %d\n", irq);
	rpigpio_toggle(RELAIS_PIN, &flag);
	return IRQ_HANDLED;
}

static int __init rpigpio_minit(void)
{
	struct device *dev;

	printk(KERN_INFO "[GRIO] Startup\n");
	spin_lock_init(&(std.lock));

	std.mjr = register_chrdev(0, MOD_NAME, &gpio_fops);
	if (std.mjr < 0) {
		printk(KERN_ALERT "[GRIO] Cannot Register");
		return std.mjr;
	}
	printk(KERN_INFO "[GRIO] Major #%d\n", std.mjr);
	
	std.cls = class_create(THIS_MODULE, "std.cls");
	if (IS_ERR(std.cls)) {
		printk(KERN_ALERT "[GRIO] Cannot get class\n");
		unregister_chrdev(std.mjr, MOD_NAME);
		return PTR_ERR(std.cls);
	}
	std.cls->devnode = st_devnode;

	dev = device_create(std.cls, NULL, MKDEV(std.mjr, 0), (void*)&std, MOD_NAME);
	if (IS_ERR(dev)) {
		printk(KERN_ALERT "[GRIO] Cannot create device\n");
		class_destroy(std.cls);
		unregister_chrdev(std.mjr, MOD_NAME);
		return PTR_ERR(dev);
	}

	release_mem_region(GPIO_BASE, 0xb4);
	if(!request_mem_region(GPIO_BASE, 0x40, MOD_NAME))
	{
		unregister_chrdev(std.mjr, MOD_NAME);
		return -ENODEV;
	}
	std.regs = ioremap(GPIO_BASE, 0x40);

	printk(KERN_INFO "[GRIO] %s loaded\n", MOD_NAME);

	std.irq = gpio_to_irq(BUTTON_PIN);
	if (request_irq(std.irq, button_int, IRQF_TRIGGER_RISING, "button_int", &std.mjr))
	{
		unregister_chrdev(std.mjr, MOD_NAME);
		return -1;
	}

	return 0;
}

static void __exit rpigpio_mcleanup(void)
{
	synchronize_irq(std.irq);
	free_irq(std.irq, &std.mjr);

	iounmap(std.regs);
	release_mem_region(GPIO_BASE, 0x40); 
	device_destroy(std.cls, MKDEV(std.mjr, 0));
	class_destroy(std.cls);
	unregister_chrdev(std.mjr, MOD_NAME);

	printk(KERN_NOTICE "[GRIO] %s removed\n", MOD_NAME);
}

module_init(rpigpio_minit);
module_exit(rpigpio_mcleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(RPIGPIO_MOD_AUTH);
MODULE_DESCRIPTION(RPIGPIO_MOD_DESC);
MODULE_SUPPORTED_DEVICE(RPIGPIO_MOD_SDEV);
