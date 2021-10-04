#ifndef __RPI_GPIO_H__
#define __RPI_GPIO_H__
//magical IOCTL number
#define GPIO_IOC_MAGIC 'k'

typedef enum {MODE_INPUT=0, MODE_OUTPUT} PIN_MODE_t;

struct gpio_data_write {
	int pin;
	char data;
};

struct gpio_data_mode {
	int pin;
	PIN_MODE_t data;
};


#define BCM2708_PERI_BASE      	 0x3F000000 // 0x20000000 // 0x3F000000
// #define GPIO_BASE                0x20000000 // 0x20200000UL //(BCM2708_PERI_BASE + 0x200000) /* GPIO */
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO */
//in: pin to read	//out: value 			//the value read on the pin
#define GPIO_READ			_IOWR(GPIO_IOC_MAGIC, 0x90, int)

//in: struct(pin, data)		//out: NONE
#define GPIO_WRITE			_IOW(GPIO_IOC_MAGIC, 0x91, struct gpio_data_write)

//in: pin to request			//out: success/fail 	// request exclusive modify privileges
#define GPIO_REQUEST		_IOW(GPIO_IOC_MAGIC, 0x92, int)

//in: pin to free
#define GPIO_FREE			_IOW(GPIO_IOC_MAGIC, 0x93, int)

//in: pin to toggle			//out: new value
#define GPIO_TOGGLE			_IOWR(GPIO_IOC_MAGIC, 0x94, int)

//in: struct (pin, mode[i/o])
#define GPIO_MODE			_IOW(GPIO_IOC_MAGIC, 0x95, struct gpio_data_mode)

//in: pin to set //set the pin (same as write 1)
#define GPIO_SET			_IOW(GPIO_IOC_MAGIC, 0x96, int)

//in: pin to clear //clear the pin (same as write 0)
#define GPIO_CLR			_IOW(GPIO_IOC_MAGIC, 0x97, int)


#endif //__RPI_GPIO_H__
