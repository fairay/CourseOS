
obj-m += modgpio.o
#systimer-objs := systimerll.o systimercore.o

CODE_DIR = client

.PHONY: client

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean: bclean


bclean:
	rm -f .*.cmd *.o *.order *.mod.c *.ko

kclean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

client:
	$(MAKE) -C $(CODE_DIR)

cclean:
	$(MAKE) -C $(CODE_DIR) clean

install: all
	sudo insmod modgpio.ko

uninstall:
	-sudo rmmod modgpio

reinstall: uninstall install