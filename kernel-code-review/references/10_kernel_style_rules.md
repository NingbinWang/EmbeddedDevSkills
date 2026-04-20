# Linux内核代码审查技能文件 - 编码与命名规范

基于Linux内核代码风格文档 (coding-style.rst) 提取的关键规则。

## 1. 缩进
- 使用制表符（Tab），每个制表符为8个字符
- 不要使用空格进行缩进（注释、文档和Kconfig除外）
- switch语句中case标签与switch对齐，不二次缩进

## 2. 行长度
- 每行不超过80列
- 长行应拆分成有意义的片段
- 不要拆分用户可见的字符串（如printk消息）

## 3. 大括号放置
- 非函数语句块（if, switch, for, while, do）：起始大括号放在行尾
  ```c
  if (x is true) {
      we do y
  }
  ```
- 函数：起始大括号放在下一行开头
  ```c
  int function(int x)
  {
      body of function
  }
  ```
- 结束大括号通常独占一行，除非后面跟着while或else

## 4. 空格使用
### 关键字后加空格
- `if`, `switch`, `case`, `for`, `do`, `while` 后加空格
- `sizeof`, `typeof`, `alignof`, `__attribute__` 后不加空格

### 操作符空格
- 二元和三元操作符两侧加空格：`= + - < > * / % | & ^ <= >= == != ? :`
- 一元操作符后不加空格：`& * + - ~ ! sizeof typeof alignof __attribute__ defined`
- 后缀自增/自减前不加空格：`i++`
- 前缀自增/自减后不加空格：`++i`
- 结构体成员操作符前后不加空格：`.` 和 `->`

### 其他
- 小括号内的表达式两侧不加空格
- 指针声明：`*` 靠近变量名而不是类型名：`char *linux_banner;`
- 行尾不留空白

## 5. 命名约定
- 局部变量名应简短：`i`, `tmp`, `count` 等
- 全局变量和函数名应具描述性
- 避免匈牙利命名法
- 避免使用 "master/slave" 和 "blacklist/whitelist"，改用替代术语

## 6. Typedef使用
- 避免对结构体和指针使用typedef
- 允许使用typedef的情况：
  - 完全不透明的对象（如pte_t）
  - 清楚的整数类型（如u8/u16/u32）
  - 使用sparse创建新类型进行类型检查
  - 与标准C99类型相同的类型
  - 用户空间安全使用的类型（如__u32）

## 7. 函数设计
- 函数应简短，只完成一件事
- 函数长度与复杂度和缩进级数成反比
- 局部变量数量不应超过5-10个
- 函数原型包含参数名和数据类型
- 使用goto进行集中的错误处理退出

## 8. 注释风格
- 使用kernel-doc格式注释API函数
- 多行注释风格：
  ```c
  /*
   * This is the preferred style for multi-line
   * comments in the Linux kernel source code.
   */
  ```
- net/和drivers/net/文件的注释风格稍有不同
- 注释数据：每行只声明一个数据，并添加简短注释

## 9. 宏定义
- 定义常量的宏名和枚举标签使用大写
- 形如函数的宏名可以使用小写
- 多语句宏应包含在do-while代码块中
- 避免影响控制流程的宏
- 宏定义中的表达式应使用小括号确保优先级

## 10. 打印内核消息
- 使用dev_err(), dev_warn(), dev_info()等设备相关宏
- 使用pr_notice(), pr_info(), pr_warn(), pr_err()等通用宏
- 拼写正确，使用完整单词（如"do not"而非"dont"）
- 消息不以句号结束
- 调试消息使用pr_debug()或dev_dbg()，默认不编译

## 11. 内存分配
- 使用kmalloc(), kzalloc(), kmalloc_array(), kcalloc(), vmalloc(), vzalloc()
- 首选形式：`p = kmalloc(sizeof(*p), ...);`
- 数组分配：`p = kmalloc_array(n, sizeof(...), ...);`
- 零初始化数组：`p = kcalloc(n, sizeof(...), ...);`
- 不要强制转换void指针返回值

## 12. 内联函数
- 谨慎使用inline，避免过度使用使内核变大
- 基本原则：3行以上的函数不要内联
- static且只使用一次的函数，gcc会自动内联

## 13. 函数返回值
- 命令性函数返回错误代码整数（0=成功，负值=失败）
- 判断性函数返回布尔值（0=失败，非0=成功）
- 返回实际结果的函数使用NULL或ERR_PTR报告错误

## 14. 布尔类型
- 使用C99 _Bool类型（bool）
- 使用true和false，而不是1和0
- 在结构体和参数中有限使用，注意对齐问题

## 15. 内核宏
- 使用内核定义的宏：ARRAY_SIZE(), sizeof_field(), min(), max()
- 不要重新发明这些宏

## 16. 条件编译
- 尽量避免在.c文件中使用#ifdef/#if
- 使用IS_ENABLED()宏将Kconfig标记转为C布尔表达式
- 在#endif后添加注释说明条件

## 17. 其他
- 不要包含编辑器模式行
- 内联汇编适当使用，用C参数
- 数据结构应有引用计数器
- Kconfig缩进：config行用制表符，help信息加2个空格

## 自动化检查工具
当使用自动化检查工具时，需要请用户提供内核的源代码路径。
- `scripts/checkpatch.pl` - 检查补丁的编码风格
- `clang-format` - 代码格式化工具
- `indent -kr -i8` - K&R风格格式化
- `scripts/Lindent` - 内核缩进脚本