#include <linux/module.h>	//module stuff
#include <linux/kernel.h>	//printk
#include <linux/init.h>		//__init
#include <linux/stat.h>		//permissions
#include <linux/device.h>	//Device class stuff
#include <linux/fs.h>		//File operation structures
#include <linux/err.h>		//Error checking macros
#include <linux/gpio.h>

#include <linux/ioctl.h>
#include <linux/io.h>		//read and write from the memory
#include <asm/uaccess.h>	//translation from userspace ptr to kernelspace
// #include <mach/platform.h>	//pull address of system timer

#include <asm/io.h>

#include <linux/types.h>	//uintxx_t

#include <linux/sched.h>	//for current->pid

#include "modgpio.h"

#define IO_OFFSET			 	0x0// 0xfd000000 // 0xfec00000 // 
#define __IO_ADDRESS(x)			((x) + IO_OFFSET)
#define IO_ADDRESS(pa)			IOMEM(__IO_ADDRESS(pa))
#define __io_address(n)		((void __iomem *)IO_ADDRESS(n))


#define RPIGPIO_MOD_AUTH "Techplex_Engineer"
#define RPIGPIO_MOD_DESC "GPIO access control for Raspberry Pi"
#define RPIGPIO_MOD_SDEV "RPiGPIO" 		//supported devices
#define MOD_NAME "rpigpio" 				//name of the device file

static int st_open(struct inode *inode, struct file *filp);
static int st_release(struct inode *inode, struct file *filp);
static long st_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);


#define PIN_NULL_PID 1	//signifies not a valid pin. Unsigned number
#define PIN_UNASSN 0	//signifies pin available
#define PIN_ARRAY_LEN 32

static volatile uint32_t *gpio_map;

//Global variables:
struct gpiomod_data {
	int mjr;
	struct class *cls;
	spinlock_t lock;
	uint32_t pins[PIN_ARRAY_LEN];
};

static struct gpiomod_data std = {
	.mjr = 0,
	.cls = NULL,
	//.lock= NULL
	.pins = {			//note this is for rpiv2
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 2
		PIN_UNASSN,	//pin 3
		PIN_UNASSN,	//pin 4
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 7
		PIN_UNASSN,	//pin 8
		PIN_UNASSN,	//pin 9
		PIN_UNASSN,	//pin 10
		PIN_UNASSN,	//pin 11
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 14
		PIN_UNASSN,	//pin 15
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 17
		PIN_UNASSN,	//pin 18
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 22
		PIN_UNASSN,	//pin 23
		PIN_UNASSN,	//pin 24
		PIN_UNASSN,	//pin 25
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 27
		PIN_UNASSN,	//pin 28
		PIN_UNASSN,	//pin 29
		PIN_UNASSN,	//pin 30
		PIN_UNASSN,	//pin 31
	}
};

//Device special files have two numbers associated with them
// -major index into array
static const struct file_operations gpio_fops = {
	.owner			= THIS_MODULE,
	.open			= st_open,
	.release 		= st_release,
	.unlocked_ioctl = st_ioctl,
};



//! handles user opening device special file
//Open and release are called each time
static int st_open(struct inode*inode, struct file *filp)
{
	return 0;
}

//! handles user closing the device special file
static int st_release(struct inode *inode, struct file *filp)
{
	int i;
	//check to see if the releasing process has any pins checked out, and free them
	spin_lock(&std.lock);
	for (i=0; i<PIN_ARRAY_LEN; i++) {
		if (std.pins[i] == current->pid) {
			printk(KERN_DEBUG "[FREE] Pin:%d From:%d\n", i, current->pid);
			std.pins[i] = PIN_UNASSN;
		}
	}
	spin_unlock(&std.lock);
	return 0;
}

//memory location defines
#define GPSET0 0x1C
#define GPCLR0 0x28
#define GPLEV0 0x34

//! Processes incomming IOCTL calls
static long st_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int pin;			//used in read, request, free
	unsigned long ret;	//return value for copy to/from user
	uint32_t val;
	uint8_t flag;
	struct gpio_data_write wdata;	//write data
	struct gpio_data_mode  mdata;	//mode data
	
	printk(KERN_DEBUG "GPIO ioctl called");
	switch (cmd) {
		case GPIO_READ:
			printk(KERN_DEBUG "here 1");
			get_user (pin, (int __user *) arg);
			printk(KERN_DEBUG "here 2 %d", pin);

			val = readl(__io_address (GPIO_BASE + GPLEV0 + (pin/32)*4)); //move to next long if pin/32 ==1
			printk(KERN_DEBUG "here 3");
			// volatile unsigned int* loc = (GPIO_BASE + GPLEV0) + (pin / 32);
			// flag = (1 << (pin % 32)) & *loc;
			flag = val >> (pin%32); 	//right shift by the remainder
			flag &= 0x01;				//clear upper bits
			printk(KERN_DEBUG "[READ] Pin: %d Val:%d\n", pin, flag);
			put_user(flag, (uint8_t __user *)arg);

			return 0;
		case GPIO_WRITE:
			//Check permissions
			ret = copy_from_user(&wdata, (struct gpio_data_write __user *)arg, sizeof(struct gpio_data_write));
			if (ret != 0) {
				printk(KERN_DEBUG "[WRITE] Error copying data from userspace\n");
				return -EFAULT;
			}
			spin_lock(&std.lock);
			if (std.pins[wdata.pin] != current->pid) { //make sure the process has it checked out
				spin_unlock(&std.lock);
				return -EACCES;	//Permission denied
			}
			spin_unlock(&std.lock);
			printk(KERN_INFO "[WRITE] Pin: %d Val:%d\n", wdata.pin, wdata.data);

			if (wdata.data == 1)
				writel (1<<wdata.pin,	__io_address (GPIO_BASE + GPSET0));
			else
				writel (1<<wdata.pin,	__io_address (GPIO_BASE + GPCLR0));

			return 0;
		case GPIO_REQUEST:
			get_user (pin, (int __user *) arg);
			spin_lock(&std.lock);
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) { //validate pins
				spin_unlock(&std.lock);
				return -EFAULT;	//Bad address
			} else if (std.pins[pin] != PIN_UNASSN) { //make sure pin is unassigined
				spin_unlock(&std.lock);
				return -EBUSY;	//Device or resource busy
			}
			std.pins[pin] = current->pid;
			spin_unlock(&std.lock);
			printk(KERN_DEBUG "[REQUEST] Pin:%d Assn To:%d\n", pin, current->pid);
			return 0;


		case GPIO_FREE:
			get_user (pin, (int __user *) arg);
			spin_lock(&std.lock);
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) { //validate pins
				spin_unlock(&std.lock);
				return -EFAULT;	//Bad address
			} else if (std.pins[pin] != current->pid) { //check access
				spin_unlock(&std.lock);
				return -EACCES;	//Permission denied
			}
			std.pins[pin] = PIN_UNASSN;
			spin_unlock(&std.lock);
			printk(KERN_DEBUG "[FREE] Pin:%d From:%d\n", pin, current->pid);
			return 0;

		case GPIO_TOGGLE:
			get_user (pin, (int __user *) arg);
			spin_lock(&std.lock);
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) { //validate pins
				spin_unlock(&std.lock);
				return -EFAULT;	//Bad address
			} else if (std.pins[pin] != current->pid) { //check access
				spin_unlock(&std.lock);
				return -EACCES;	//Permission denied
			}
			spin_unlock(&std.lock);
			val = readl(__io_address (GPIO_BASE + GPLEV0 + (pin/32)*4)); //move to next long if pin/32 ==1
			flag = val >> (pin%32); 	//right shift by the remainder
			flag &= 0x01;				//clear upper bits
			printk(KERN_DEBUG "[TOGGLE] Pin:%d From:%.1d To:%.1d\n", pin, flag, flag?0:1);
			if (flag != 1)//set
				writel (1<<pin,	__io_address(GPIO_BASE + GPSET0));
			else //clear
				writel (1<<pin,	__io_address(GPIO_BASE + GPCLR0));
			put_user(flag?0:1, (uint8_t __user *)arg);
			return 0;

		case GPIO_MODE:
			ret = copy_from_user(&mdata, (struct gpio_data_mode __user *)arg, sizeof(struct gpio_data_mode));
			if (ret != 0) {
				printk(KERN_DEBUG "[MODE] Error copying data from userspace\n");
				return -EFAULT;
			}
			spin_lock(&std.lock);
			if (mdata.pin > 31 || mdata.pin < 0 || std.pins[mdata.pin] == PIN_NULL_PID) { //Validate Pin
				spin_unlock(&std.lock);
				return -EFAULT;	//Bad address
			} else if (std.pins[mdata.pin] != current->pid) { //check access
				spin_unlock(&std.lock);
				return -EACCES;	//Permission denied
			}
			spin_unlock(&std.lock);

			//clear the bits (sets to input)
			writel(~(7<<((mdata.pin %10)*3)) & readl(__io_address(GPIO_BASE + (0x04)*(mdata.pin /10))), __io_address(GPIO_BASE + (0x04)*(mdata.pin/10)));
			if(mdata.data == MODE_INPUT) {
				printk(KERN_DEBUG "[MODE] Pin %d set as Input\n", mdata.pin);
			} else if (mdata.data == MODE_OUTPUT) {
				writel(1<<((mdata.pin % 10)*3) | readl(__io_address(GPIO_BASE + (0x04)*(mdata.pin/10))), __io_address(GPIO_BASE + (0x04)*(mdata.pin/10)));    // Set pin as output
				printk(KERN_DEBUG "[MODE] Pin %d set as Output\n", mdata.pin);
			} else {
				return -EINVAL;	//Invalid argument
			}
			return 0;
		case GPIO_SET:
		case GPIO_CLR:

			get_user (pin, (int __user *) arg);
			spin_lock(&std.lock);
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) { //validate pins
				spin_unlock(&std.lock);
				return -EFAULT;	//Bad address
			} else if (std.pins[pin] != current->pid) { //check access
				spin_unlock(&std.lock);
				return -EACCES;	//Permission denied
			}
			spin_unlock(&std.lock);

			if (cmd == GPIO_SET) {
				writel (1<<pin,	__io_address (GPIO_BASE + GPSET0));
				printk(KERN_INFO "[SET] Pin: %d\n", pin);
			} else {
				writel (1<<pin,	__io_address (GPIO_BASE + GPCLR0));
				printk(KERN_INFO "[CLR] Pin: %d\n", pin);
			}

			return 0;
		default:
			return -ENOTTY;	//Error Message: inappropriate IOCTL for device
	}
}

//! Sets permissions correctly on created device special file
static char *st_devnode(struct device *dev, umode_t *mode)
{
	if (mode) *mode = 0666;//adding a leading 0 makes number octal
	return NULL;
}
//! initialize the driver.
/*!
Couple of things to do here:

1. register char device
2. Create class
3. Create device

*/
static int __init rpigpio_minit(void)
{
	struct device *dev;

	printk(KERN_INFO "[gpio] Startup\n");
	//register char device
	std.mjr = register_chrdev(0, MOD_NAME, &gpio_fops);//character device
	if (std.mjr < 0) {
		printk(KERN_ALERT "[gpio] Cannot Register");
		return std.mjr;
	}
	printk(KERN_INFO "[gpio] Major #%d\n", std.mjr);

	std.cls = class_create(THIS_MODULE, "std.cls");
	if (IS_ERR(std.cls)) {
		printk(KERN_ALERT "[gpio] Cannot get class\n");
		//Need to unregister
		unregister_chrdev(std.mjr, MOD_NAME);
		return PTR_ERR(std.cls);					//Gets errrno code so one can lookup the error
	}

	//store a pointer to the st_devnode function
	//-its a callback when the device special file is actually being created
	std.cls->devnode = st_devnode;

	//Create Device													name of dev/spec.file
	dev = device_create(std.cls, NULL, MKDEV(std.mjr, 0), (void*)&std, MOD_NAME);
	if (IS_ERR(dev)) {
		printk(KERN_ALERT "[gpio] Cannot create device\n");
		//remove classs
		class_destroy(std.cls);
		//Unregister
		unregister_chrdev(std.mjr, MOD_NAME);
		return PTR_ERR(dev);
	}
	printk(KERN_INFO "[gpio] %s Loaded\n", MOD_NAME);

	//init the spinlock
	spin_lock_init(&(std.lock));

	return 0;
}

static void __exit rpigpio_mcleanup(void)
{
	device_destroy(std.cls, MKDEV(std.mjr, 0));
	class_destroy(std.cls);
	unregister_chrdev(std.mjr, MOD_NAME);

	printk(KERN_NOTICE "[gpio] Goodbye\n");
}

module_init(rpigpio_minit);
module_exit(rpigpio_mcleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(RPIGPIO_MOD_AUTH);
MODULE_DESCRIPTION(RPIGPIO_MOD_DESC);
MODULE_SUPPORTED_DEVICE(RPIGPIO_MOD_SDEV);
