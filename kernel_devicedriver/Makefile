KDIR = /lib/modules/`uname -r`/build

#obj-m := hello_module.o
#obj-m := gpio_module.o
#obj-m := gpiofunction_module.o
#obj-m := gpioirq_module.o
#obj-m := gpiotimer_module.o
obj-m := gpiosignal_module.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
