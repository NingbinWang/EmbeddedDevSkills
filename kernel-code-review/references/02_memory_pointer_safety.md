# 代码审查技能文件 - 内存与指针安全

本文档例举Linux C(内核与驱动)安全编码规范中内存与指针安全相关条款。

## 一、内存与指针安全

### 1.1 分配内存必须检查返回值

**【描述】**
内核态通过 `kmalloc` / `vmalloc` / `kzalloc` 等接口分配内存可能失败，必须严格检查返回的指针是否为空。

**【风险】**
空指针解引用导致内核崩溃。

**【错误代码示例】**
```c
struct my_struct *item = kmalloc(sizeof(*item), GFP_KERNEL);
item->id = 1; // 错误：未判断是否为NULL
```

**【正确代码示例】**
```c
struct my_struct *item = kmalloc(sizeof(*item), GFP_KERNEL);
if (!item) {
    return -ENOMEM;
}
item->id = 1;
```

### 1.2 释放后指针必须置空或不再使用

**【描述】**
指针指向的内存被 `kfree` 释放后，如果继续访问，会导致Use-After-Free (UAF)漏洞。

**【错误代码示例】**
```c
kfree(ptr);
// ...
if (condition) {
    ptr->flag = 1; // 错误：UAF
}
```

**【正确代码示例】**
```c
kfree(ptr);
ptr = NULL; // 防御防范
```

### 1.3 不要解引用未校验的用户空间指针

**【描述】**
在内核中直接解引用用户空间传来的指针是极其危险且错误的。必须使用专用的接口，如 `copy_from_user`，`copy_to_user`，`get_user`，`put_user`。

**【风险】**
任意地址读写漏洞，内核崩溃，或者安全绕过。

**【错误代码示例】**
```c
long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct my_data *data = (struct my_data *)arg;
    int val = data->val; // 错误：直接解引用用户指针
    return 0;
}
```

**【正确代码示例】**
```c
long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct my_data data;
    if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
        return -EFAULT;
    }
    int val = data.val;
    return 0;
}
```

### 1.4 避免数组越界访问

**【描述】**
使用外部输入作为数组的索引时，必须确保索引在 [0, ARRAY_SIZE-1] 范围内。

**【正确代码示例】**
```c
int idx = ... // 输入
if (idx < 0 || idx >= ARRAY_SIZE(my_array)) {
    return -EINVAL;
}
int val = my_array[idx];
```

### 1.5 严禁将局部栈变量的地址返回到其作用域之外

**【描述】**
局部自动变量（在栈上分配）的生命周期只存在于定义它的函数或代码块内。如果将它的地址（指针）返回、或者赋值给全局指针并在之后被访问，那将读取到无效或被覆盖的内存，引发极其危险的安全漏洞（如任意代码执行）。

****【错误代码示例】****
```c
struct dev_info *get_dev_info(void) {
    struct dev_info info; // 栈上变量
    info.id = 1;
    info.status = DEV_OK;
    return &info; // 错误：返回栈变量地址，发生悬空指针
}
```

### 1.6 避免使用未初始化的局部变量

**【描述】**
在C语言中，未静态初始化的局部栈变量的值是不确定的。必须在首次使用变量（读取其值）前对其进行显式初始化。在内核里，未初始化的栈变量泄漏到用户态(Uninitialized stack leak) 是信息泄露的高发漏洞。

**【正确代码示例】**
```c
// 定义时就清零，避免数据残留
struct my_data data = {0}; 
// 或者是
memset(&data, 0, sizeof(data));
```

### 1.7 正确使用结构体尾部的柔性数组

**【描述】**
对于包含零长数组或柔性数组成员的结构体，在申请内存时必须计算结构体本身和尾部数组内存的总和，并通过 `struct_size()` 宏来防止整数溢出。

**【正确代码示例】**
```c
struct pkt {
    int len;
    u8 data[];
};

size_t data_len = ...; // 来自用户态
struct pkt *p = kmalloc(struct_size(p, data, data_len), GFP_KERNEL);
if (!p) return -ENOMEM;
```
