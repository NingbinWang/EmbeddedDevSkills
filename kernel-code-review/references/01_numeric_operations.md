# 代码审查技能文件 - 数值运算安全

本文档例举Linux C(内核与驱动)安全编码规范中数值运算安全相关条款, 为代码检视过程提供编码规范指导。

## 一、数值运算安全

数值运算安全涉及整数溢出、回绕、除零错误等关键安全问题，在内核态尤为重要，往往是内核提权漏洞的根源。

### 1.1 确保有符号整数运算不溢出

**【描述】**
有符号整数溢出是未定义的行为。在内核态处理来自用户态的输入参数时，若存在加、减、乘等运算，一定要检查是否会产生溢出。

**【风险】**
整数溢出可导致计算出的内存大小远小于预期，从而引发堆溢出等问题。

**【错误代码示例】**
```c
int len1 = ... // 来自用户态
int len2 = ... // 来自用户态
// 可能发生整数溢出，导致 total_len 变成负数或较小的正数
int total_len = len1 + len2; 
void *buf = kmalloc(total_len, GFP_KERNEL); // 分配过小的内存
```

**【正确代码示例】**
```c
int len1 = ... // 来自用户态
int len2 = ... // 来自用户态
int total_len;
if (__builtin_sadd_overflow(len1, len2, &total_len)) {
    return -EINVAL; // 处理溢出情况
}
void *buf = kmalloc(total_len, GFP_KERNEL);
```

### 1.2 确保无符号整数运算不回绕

**【描述】**
无符号操作数的计算超出表示范围会按照（结果类型可表示的最大值 + 1）的数值取模，即回绕。

**【风险】**
常发生在内核对用户态传入的指针偏移、长度等进行计算时，产生条件绕过。

**【错误代码示例】**
```c
size_t len = ... // 来自用户态
// 若 len 特别大，加法将发生回绕，从而绕过校验
if (len + sizeof(struct header) > MAX_SIZE) {
    return -EINVAL;
}
```

**【正确代码示例】**
```c
size_t len = ... // 来自用户态
if (len > MAX_SIZE - sizeof(struct header)) {
    return -EINVAL;
}
```

### 1.3 防止除零异常

**【描述】**
除零异常在内核中通常会导致内核Oops或Panic，造成拒绝服务(DoS)。

**【风险】**
恶意用户态程序传递0作为除数。

**【正确代码示例】**
```c
int divisor = ... // 来自用户态
if (divisor == 0) {
    return -EINVAL;
}
int result = value / divisor;
```

### 1.4 位运算安全

**【描述】**
进行位偏移操作时，位移量不得超过目标类型的位宽；对有符号数进行左移时要注意符号位的影响。尽量使用无符号类型进行位操作。

**【正确代码示例】**
```c
unsigned int val = 1;
unsigned int shift = ... // 来自外部输入
if (shift >= sizeof(val) * 8) {
    return -EINVAL;
}
unsigned int result = val << shift;
```
