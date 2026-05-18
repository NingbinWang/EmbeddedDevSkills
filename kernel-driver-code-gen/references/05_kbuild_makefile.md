# Kbuild / Makefile 参考

## 外部模块编译（Out-of-tree）

### 单文件模块

```makefile
# Makefile
obj-m := mydrv.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

### 多文件模块

```makefile
# Makefile
mydrv-objs := main.o hw.o utils.o
obj-m := mydrv.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

### 交叉编译

```makefile
# ARM 交叉编译
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
KDIR ?= /path/to/linux-kernel

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules
```

### 多模块

```makefile
obj-m := mod_a.o mod_b.o

# 或者带子目录
obj-m := parent/
```

## Kconfig（集成到内核树）

```
# drivers/mydrv/Kconfig
config MY_DRIVER
    tristate "My custom driver"
    depends on OF  # 依赖设备树
    default m
    help
      This is my custom driver for XYZ hardware.
      Say Y to build into kernel, M for module.

config MY_DRIVER_DEBUG
    bool "My driver debug support"
    depends on MY_DRIVER
    default n
    help
      Enable debug output for my driver.
```

### Makefile 引用 Kconfig

```makefile
# drivers/mydrv/Makefile
obj-$(CONFIG_MY_DRIVER) += mydrv.o
mydrv-objs := main.o hw.o

# 调试功能
mydrv-$(CONFIG_MY_DRIVER_DEBUG) += debug.o
```

## 内核模块 Makefile 模式

### 条件编译

```makefile
# 根据内核版本条件编译
KERNEL_VERSION := $(shell uname -r | cut -d. -f1-2)

ifeq ($(shell test $(KERNEL_VERSION) \>= 6.1; echo $$?),0)
ccflags-y += -DKERNEL_6_1_PLUS
endif
```

### 自定义编译选项

```makefile
ccflags-y += -DDEBUG
ccflags-y += -I$(src)/include
CFLAGS_main.o += -Wno-unused-variable
```

### 安装

```makefile
INSTALL_MOD_DIR ?= extra

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install INSTALL_MOD_DIR=$(INSTALL_MOD_DIR)
	depmod -a
```

## 模块加载/卸载

```bash
# 编译
make

# 加载
sudo insmod mydrv.ko

# 查看已加载模块
lsmod | grep mydrv

# 查看模块信息
modinfo mydrv.ko

# 查看内核日志
dmesg | tail -20

# 卸载
sudo rmmod mydrv

# 带参数加载
sudo insmod mydrv.ko debug=1 baudrate=115200

# 自动加载（安装后）
sudo modprobe mydrv
```

## 模块参数

```c
#include <linux/moduleparam.h>

static int debug = 0;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Enable debug output (0=off, 1=on)");

static char *name = "default";
module_param(name, charp, 0644);
MODULE_PARM_DESC(name, "Device name");

/* 数组参数 */
static int channels[4] = {0, 1, 2, 3};
static int nr_channels;
module_param_array(channels, int, &nr_channels, 0644);
```

```bash
# 运行时修改参数
echo 1 | sudo tee /sys/module/mydrv/parameters/debug
cat /sys/module/mydrv/parameters/debug
```

## Makefile 最佳实践

```makefile
# 完整的外部模块 Makefile
MODULE_NAME := mydrv

$(MODULE_NAME)-objs := main.o hw.o utils.o
obj-m := $(MODULE_NAME).o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# 默认目标
all: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

# 开发用：卸载旧模块 → 编译 → 加载
reload:
	-sudo rmmod $(MODULE_NAME) 2>/dev/null
	$(MAKE) clean
	$(MAKE) modules
	sudo insmod $(MODULE_NAME).ko

# 查看日志
log:
	dmesg | tail -20

.PHONY: all modules clean install reload log
```

## 常见陷阱

1. **obj-m vs obj-y** — obj-m 编译为模块，obj-y 编译进内核
2. **-objs 后缀** — `mydrv-objs` 表示 mydrv.o 由这些文件链接而成（注意是 - 不是 _）
3. **KDIR 必须指向内核构建目录** — 不是源码目录，通常是 `/lib/modules/$(uname -r)/build`
4. **交叉编译必须同时设置 ARCH 和 CROSS_COMPILE** — 否则用宿主机编译器
5. **modules_install 后需要 depmod** — 否则 modprobe 找不到模块
