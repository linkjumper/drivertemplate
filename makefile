ifneq ($(KERNELRELEASE),)
	obj-m	:= mod.o

else
	KDIR	:= /lib/modules/$(shell uname -r)/build
	PWD	:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
endif

clean:
	rm -rf *.ko *.o .*.cmd .tmp_versions Module.symvers
	rm -rf modules.order *.mod.c
