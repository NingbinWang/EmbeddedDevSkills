# 代码审查技能文件 - 敏感信息保护

本文档参考华为C语言安全编程规范，例举Linux C(内核与驱动)中保护敏感信息的相关安全条款。

## 一、敏感信息保护安全

### 1.1 禁止硬编码敏感信息

**【描述】**
代码中绝对不能以明文形式硬编码密码、密钥、Token、加密盐值等高安全敏感信息。

**【风险】**
无论在各种环境和版本中，一旦代码泄露或被反编译，攻击者可以轻易提取这些敏感数据，严重危害系统安全。

**【错误代码示例】**
```c
// 错误：在代码中硬编码了连接设备的密码
#define DEVICE_AUTH_KEY "Admin!@#123"

int authenticate_device(const char *key) {
    if (strcmp(key, DEVICE_AUTH_KEY) == 0) { ... }
}
```

**【正确防御】**
必须依赖安全的密钥管理机制，或者从安全的内核Key Retention Service (`keyrings`)、安全环境(如TEE)获取。

### 1.2 敏感数据在内存中释放前须彻底清除

**【描述】**
在使用完密码、密钥或用户私密信息后，应当立即清理内存（覆盖写入零），以防相关内容驻留于栈帧、堆内存中。

**【风险】**
如果敏感数据未清除便被释放，可能通过核心转储 (core dump/crash dump) 或未初始化内存泄露 (Uninitialized Memory Disclosure) 被攻击者窃取。

**【错误代码示例】**
```c
void process_crypto_key(void) {
    char key[32];
    get_key_from_user(key, sizeof(key));
    do_crypto_op(key);
    // 错误：函数结束直接返回，栈内存上残留了明文key
}
```

**【正确代码示例】**
```c
#include <linux/string.h>

void process_crypto_key(void) {
    char key[32];
    get_key_from_user(key, sizeof(key));
    do_crypto_op(key);
    
    // 内核应当使用 memzero_explicit (类似于 memset_s) 确保编译器不优化掉清除过程
    memzero_explicit(key, sizeof(key));
}
```

### 1.3 禁止在日志中打印输出敏感信息

**【描述】**
禁止通过 `printk`、`dev_err` 等系统日志接口打印用户密码、对称密钥、详细的非公开网络拓扑等隐私或敏感数据。日志可以通过 `dmesg` 或 `/var/log/messages` 被无特权用户读取。
