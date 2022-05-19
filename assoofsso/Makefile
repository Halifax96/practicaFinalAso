obj-m := assoofs.o
#KERNEL := 5.13.0-39-generic


all: ko mkassoofs

ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

mkassoofs_SOURCES:
	mkassoofs.c assoofs.h

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm mkassoofs
