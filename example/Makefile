obj-m += phc.o

#Disabling module signature
#CONFIG_MODULE_SIG=n

ifdef ARCH
    KDIR = /home/abc/def
else
    KDIR = /lib/modules/$(shell uname -r)/build
endif

all:
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean

