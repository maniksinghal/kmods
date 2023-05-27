obj-m += ptp_clock.o

#Disabling module signature
#CONFIG_MODULE_SIG=n

ifdef ARCH
    KDIR = /home/abc/def
else
    KDIR = /lib/modules/$(shell uname -r)/build
endif

clock_test:  clock_test.c
	gcc clock_test.c -o clock_test

test: clock_test

module: 
	make -C $(KDIR) M=$(shell pwd) modules

all: module test

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	rm -rf clock_test

