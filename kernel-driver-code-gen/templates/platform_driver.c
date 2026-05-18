/*
 * 平台驱动模板（设备树匹配）
 * 编译: make
 * 加载: sudo insmod platform_driver.ko
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/interrupt.h>

#define DRIVER_NAME "my-platform-drv"

struct mydrv_priv {
    struct device *dev;
    void __iomem *regs;
    int irq;
};

static irqreturn_t mydrv_isr(int irq, void *data) {
    struct mydrv_priv *priv = data;
    u32 status = readl(priv->regs + 0x00);
    if (!status) return IRQ_NONE;
    writel(status, priv->regs + 0x04);
    dev_info(priv->dev, "irq fired: 0x%x\n", status);
    return IRQ_HANDLED;
}

static int mydrv_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct mydrv_priv *priv;
    struct resource *res;
    int irq, ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv) return -ENOMEM;
    priv->dev = dev;
    platform_set_drvdata(pdev, priv);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) { dev_err(dev, "no mem resource\n"); return -ENODEV; }
    priv->regs = devm_ioremap_resource(dev, res);
    if (IS_ERR(priv->regs)) return PTR_ERR(priv->regs);

    irq = platform_get_irq(pdev, 0);
    if (irq < 0) return irq;
    ret = devm_request_irq(dev, irq, mydrv_isr, 0, DRIVER_NAME, priv);
    if (ret) { dev_err(dev, "request irq failed: %d\n", ret); return ret; }

    priv->irq = irq;
    dev_info(dev, "probe ok, irq=%d\n", irq);
    return 0;
}

static void mydrv_remove(struct platform_device *pdev) {
    dev_info(&pdev->dev, "removed\n");
}

static const struct of_device_id mydrv_of_match[] = {
    { .compatible = "vendor,my-device" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mydrv_of_match);

static struct platform_driver mydrv_driver = {
    .probe  = mydrv_probe,
    .remove = mydrv_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = mydrv_of_match,
    },
};

module_platform_driver(mydrv_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Platform driver template with device tree");
