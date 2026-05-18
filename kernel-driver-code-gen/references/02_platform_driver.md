# 平台驱动参考

## platform_driver 框架

### 基本结构

```c
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/module.h>

static int mydrv_probe(struct platform_device *pdev);
static void mydrv_remove(struct platform_device *pdev);

/* 设备树匹配表 */
static const struct of_device_id mydrv_of_match[] = {
    { .compatible = "vendor,my-device" },
    { .compatible = "vendor,my-device-v2" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mydrv_of_match);

/* ACPI 匹配表（可选） */
static const struct acpi_device_id mydrv_acpi_match[] = {
    { "MYDEV0001", 0 },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(acpi, mydrv_acpi_match);

/* 平台驱动结构 */
static struct platform_driver mydrv_driver = {
    .probe  = mydrv_probe,
    .remove = mydrv_remove,
    .driver = {
        .name = "my-driver",
        .of_match_table = mydrv_of_match,
        .acpi_match_table = ACPI_PTR(mydrv_acpi_match),
    },
};

module_platform_driver(mydrv_driver);
```

## probe 函数

### 获取硬件资源

```c
static int mydrv_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct mydrv_priv *priv;
    struct resource *res;
    int irq, ret;

    /* 分配私有数据 */
    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = dev;
    platform_set_drvdata(pdev, priv);

    /* ---- 获取内存资源 ---- */

    /* 方式1：从设备树 reg 属性获取 */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(dev, "no memory resource\n");
        return -ENODEV;
    }

    /* 映射寄存器 */
    priv->regs = devm_ioremap_resource(dev, res);
    if (IS_ERR(priv->regs))
        return PTR_ERR(priv->regs);

    /* 方式2：简写 */
    priv->regs = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(priv->regs))
        return PTR_ERR(priv->regs);

    /* ---- 获取中断 ---- */

    irq = platform_get_irq(pdev, 0);
    if (irq < 0)
        return irq;

    ret = devm_request_irq(dev, irq, mydrv_isr,
                           IRQF_SHARED, dev_name(dev), priv);
    if (ret) {
        dev_err(dev, "failed to request irq %d: %d\n", irq, ret);
        return ret;
    }

    /* ---- 获取可选资源 ---- */

    /* GPIO */
    priv->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
    if (IS_ERR(priv->reset_gpio))
        return PTR_ERR(priv->reset_gpio);

    /* 时钟 */
    priv->clk = devm_clk_get(dev, NULL);
    if (IS_ERR(priv->clk))
        return PTR_ERR(priv->clk);

    ret = clk_prepare_enable(priv->clk);
    if (ret)
        return ret;

    /* DMA 通道 */
    ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
    if (ret) {
        dev_err(dev, "failed to set DMA mask\n");
        goto err_clk;
    }

    /* ---- 注册功能 ---- */

    ret = mydrv_register_chardev(priv);
    if (ret)
        goto err_clk;

    dev_info(dev, "probe successful\n");
    return 0;

err_clk:
    clk_disable_unprepare(priv->clk);
    return ret;
}
```

## remove 函数

```c
static void mydrv_remove(struct platform_device *pdev)
{
    struct mydrv_priv *priv = platform_get_drvdata(pdev);

    /* 逆序释放资源 */
    mydrv_unregister_chardev(priv);
    clk_disable_unprepare(priv->clk);

    dev_info(&pdev->dev, "removed\n");

    /* devm_ 资源自动释放，无需手动处理 */
}
```

## 设备树属性读取

```c
struct device_node *np = dev->of_node;
u32 val;
const char *str;
int count;

/* 整数属性 */
if (of_property_read_u32(np, "reg-offset", &val)) {
    dev_err(dev, "missing reg-offset\n");
    return -EINVAL;
}

/* 字符串属性 */
if (of_property_read_string(np, "label", &str) == 0)
    dev_info(dev, "label: %s\n", str);

/* 布尔属性 */
if (of_property_read_bool(np, "use-dma"))
    priv->use_dma = true;

/* 数组属性 */
u32 channels[4];
count = of_property_read_variable_u32_array(np, "channels",
                                            channels, 1, 4);
if (count < 0)
    return count;

/* 枚举属性 */
const char *mode_str;
if (of_property_read_string(np, "operating-mode", &mode_str) == 0) {
    if (strcmp(mode_str, "high-speed") == 0)
        priv->mode = MODE_HIGH_SPEED;
    else if (strcmp(mode_str, "low-power") == 0)
        priv->mode = MODE_LOW_POWER;
}
```

## 设备树节点示例

```dts
/ {
    soc {
        my_device@10000000 {
            compatible = "vendor,my-device";
            reg = <0x10000000 0x1000>;  /* 寄存器基地址和大小 */
            interrupts = <0 45 4>;       /* GIC SPI 45, 高电平触发 */
            clocks = <&clk_50m>;
            clock-names = "apb";
            reset-gpios = <&gpio1 5 GPIO_ACTIVE_LOW>;
            status = "okay";

            /* 自定义属性 */
            operating-mode = "high-speed";
            channels = <0 1 2 3>;
            use-dma;
        };
    };
};
```

## platform_driver vs 字符设备

| 方面 | platform_driver | 字符设备 |
|------|----------------|----------|
| 匹配方式 | 设备树 / ACPI / 名称 | 设备号 |
| 生命周期 | probe / remove | init / exit |
| 资源管理 | devm_ 自动 | 手动 |
| 适用场景 | 有硬件设备的驱动 | 纯软件设备 / 虚拟设备 |

**常见模式**：platform_driver 内部注册字符设备。probe 中获取硬件资源并注册 cdev，remove 中注销 cdev 并释放资源。

## 常见陷阱

1. **compatible 字符串必须与设备树完全匹配** — 包括大小写
2. **platform_get_irq 返回负数表示错误** — 不是返回 0
3. **devm_ioremap_resource 返回 ERR_PTR** — 用 IS_ERR / PTR_ERR 检查
4. **clk_prepare_enable 失败需要回滚** — 不要忽略返回值
5. **of_match_table 必须有 sentinel** — 空的 `{ }` 结尾
