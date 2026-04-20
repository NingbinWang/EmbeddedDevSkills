# 代码审查技能文件 - 输入验证

本文档例举Linux C(内核与驱动)安全编码规范中输入验证相关条款。

## 一、输入验证

### 1.1 copy_from_user / copy_to_user 返回值必须校验

**【描述】**
与用户空间进行数据交互时，必须使用受保护的API并检查其返回值。`copy_from_user` 失败时会返回未能成功拷贝的字节数，而不是错误码。通常只要返回值非0，就应当向用户层返回 `-EFAULT`。

**【风险】**
对未成功拷贝的无效内核内存进行操作，可造成信息泄露或控制流劫持。

**【错误代码示例】**
```c
struct my_data data;
// 错误：不检查返回值
copy_from_user(&data, user_ptr, sizeof(data));
// 若拷贝失败，data的内容是未初始化的栈内存
```

**【正确代码示例】**
```c
struct my_data data;
if (copy_from_user(&data, user_ptr, sizeof(data))) {
    return -EFAULT;
}
```

### 1.2 ioctl 参数与长度验证

**【描述】**
处理用户态通过 `ioctl` 传递的数据结构，需对其内部所有的长度指示符（length/size）和偏移（offset）字段进行严格校验，确保不会超出物理/逻辑边界。

**【错误代码示例】**
```c
struct req {
    u32 len;
    u8 __user *buf;
};
long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct req r;
    if (copy_from_user(&r, (void __user *)arg, sizeof(r)))
        return -EFAULT;
    
    // 错误：未限制 r.len 的最大值，可能引发大块内存分配攻击
    void *kbuf = kmalloc(r.len, GFP_KERNEL); 
}
```

**【正确代码示例】**
```c
if (r.len > MAX_ALLOWED_LEN) {
    return -EINVAL;
}
void *kbuf = kmalloc(r.len, GFP_KERNEL);
```

### 1.3 注意 TOCTOU (Time-Of-Check to Time-Of-Use) 漏洞

**【描述】**
如果要从用户态读取两次相同的数据结构中的某个字段（比如第一次读长度，第二次读数据），恶意用户线程可能会在两次读取间隙修改内存值，造成双重获取 (Double Fetch) 漏洞。

**【合规要求】**
将数据一次性全部 `copy_from_user` 到内核结构数组中，然后所有的解析、判断只针对已经拷贝到内核态的副本，不再进行第二次 `copy_from_user` 读取同一位置。
