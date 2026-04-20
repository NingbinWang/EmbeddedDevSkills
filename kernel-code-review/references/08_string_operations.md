# 代码审查技能文件 - 字符串安全操作

本文档参考华为C语言编程安全规范，例举Linux C(内核与驱动)中字符串操作及格式化相关安全条款。

## 一、字符串与格式化安全

### 1.1 遵循防御性编程原则，避免缓冲区溢出

**【描述】**
字符串的拷贝和拼接操作，必须进行严格的边界检查。禁止使用不检查长度而直接拷贝的危险函数。

**【内核态要求】**
- 禁止使用 `strcpy`, `strcat`, `sprintf` 等危险函数。
- 应当使用更加安全的内核变体如 `strscpy`, `strlcpy`（已在很多内核版本中被 `strscpy` 替代）, `snprintf`。

**【错误代码示例】**
```c
char buf[16];
// 错误：未经长度校验直接拷贝，可能造成栈溢出
strcpy(buf, user_input_str); 
```

**【正确代码示例】**
```c
char buf[16];
// 使用 strscpy 或 strncpy 并确保以 \0 收尾
strscpy(buf, user_input_str, sizeof(buf));
```

### 1.2 防止格式化字符串漏洞

**【描述】**
不要将不可信的数据直接作为 `printk`, `snprintf` 等格式化函数的 `format` 参数。

**【风险】**
攻击者通过传入带有 `%x`, `%n` 等格式化符号的字符串，可以实现越权读取甚至覆盖内存数据，造成信息泄露或内核破坏。

**【错误代码示例】**
```c
char user_input[] = "%s%p...”; // 来自用户空间的恶意字符串
printk(user_input);          // 错误：直接将用户可控字符串作为format参数
```

**【正确代码示例】**
```c
char user_input[] = "%s%p...";
printk(KERN_INFO "%s", user_input); // 正确：使用%s打印可变字符串
```

### 1.3 确保字符串始终是以 null ('\0') 结尾的

**【描述】**
在使用字符串操作API或向其传参时，必须确保字符串含有结束符。很多内核接口或C标准库接口需要遇到 `\0` 才停止，否则会引起越界读取 (Out-of-bounds Read)。特别是使用 `strncpy` 时，当源字符串长度大于等于目的缓冲区大小时，目标缓冲区并不会被自动加上 `\0`。

**【正确代码示例】**
```c
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```
*(注：在Linux内核中更推荐直接使用 `strscpy`(dest, src, sizeof(dest)))*
