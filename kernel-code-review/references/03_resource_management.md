# 代码审查技能文件 - 资源管理

本文档例举Linux C(内核与驱动)安全编码规范中资源管理相关条款。

## 一、资源管理

### 1.1 避免内存泄漏

**【描述】**
所有分配的内存必须在所有的退出路径上得到正确释放，尤其是发生错误处理(Error Handling)的分支。

**【风险】**
内核不断消耗内存，最终导致OOM，系统崩溃。

**【错误代码示例】**
```c
struct obj *p = kmalloc(sizeof(*p), GFP_KERNEL);
if (!p) return -ENOMEM;

if (do_something(p) < 0) {
    return -EINVAL; // 错误：未释放 p 导致内存泄漏
}
kfree(p);
return 0;
```

**【正确代码示例】**
```c
// 推荐使用goto统一错误处理路径
struct obj *p = kmalloc(sizeof(*p), GFP_KERNEL);
if (!p) return -ENOMEM;

if (do_something(p) < 0) {
    kfree(p);
    return -EINVAL; 
}

kfree(p);
return 0;
```

### 1.2 避免锁泄漏

**【描述】**
一旦获取了锁（如spin_lock、mutex_lock），在任何情况下函数退出前必须释放锁。

**【错误代码示例】**
```c
spin_lock(&my_lock);
if (error_condition) {
    return -EINVAL; // 错误：未释放锁即返回，容易引发死锁
}
// 处理逻辑
spin_unlock(&my_lock);
return 0;
```

### 1.3 避免文件描述符泄漏/引用计数泄漏

**【描述】**
内核中频繁使用引用计数(`kref_get`/`kref_put`)或者获取对象指针（`fget`/`fput`）。每次获取引用都必须在匹配的作用域内被正确释放。

**【正确代码示例】**
```c
struct file *f = fget(fd);
if (!f) return -EBADF;

if (check_something(f)) {
    fput(f);
    return -EINVAL;
}

fput(f);
return 0;
```
