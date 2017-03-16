ifneq ($(KERNELRELEASE),)
	obj-m	:= mod.o
#	obj-m	:= ioctl.o
#	obj-m	:= irq.o
#	obj-m	:= threaded_irq.o	
#	obj-m	:= proc.o	

else
	KDIR	:= /lib/modules/$(shell uname -r)/build
	PWD	:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
endif

clean:
	rm -rf *.ko *.o .*.cmd .tmp_versions Module.symvers
	rm -rf modules.order *.mod.c
