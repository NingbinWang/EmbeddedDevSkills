# 内核模块 Makefile 模板
#
# 单文件模块:
#   obj-m := mydrv.o
# 多文件模块:
#   mydrv-objs := main.o hw.o utils.o
#   obj-m := mydrv.o

MODULE_NAME := mydrv

# 多文件模块取消注释:
# $(MODULE_NAME)-objs := main.o hw.o

obj-m := $(MODULE_NAME).o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# 交叉编译（取消注释并修改）:
# ARCH ?= arm64
# CROSS_COMPILE ?= aarch64-linux-gnu-

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

reload:
	-sudo rmmod $(MODULE_NAME) 2>/dev/null
	$(MAKE) clean
	$(MAKE) modules
	sudo insmod $(MODULE_NAME).ko

log:
	dmesg | tail -20

.PHONY: all modules clean install reload log
