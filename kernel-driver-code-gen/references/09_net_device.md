# 网络设备驱动参考

## 基本框架

```c
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#define DRV_NAME "mynet"

struct mynet_priv {
    struct net_device *netdev;
    struct napi_struct napi;
    void __iomem *regs;
    int irq;
    /* 硬件相关 */
};
```

## net_device 初始化

```c
static int mynet_open(struct net_device *dev);
static int mynet_stop(struct net_device *dev);
static netdev_tx_t mynet_xmit(struct sk_buff *skb, struct net_device *dev);
static int mynet_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
static struct net_device_stats *mynet_get_stats(struct net_device *dev);

static const struct net_device_ops mynet_netdev_ops = {
    .ndo_open       = mynet_open,
    .ndo_stop       = mynet_stop,
    .ndo_start_xmit = mynet_xmit,
    .ndo_do_ioctl   = mynet_ioctl,
    .ndo_get_stats  = mynet_get_stats,
    .ndo_set_rx_mode = mynet_set_rx_mode,
    .ndo_set_mac_address = eth_mac_addr,
    .ndo_validate_addr   = eth_validate_addr,
};

static const struct ethtool_ops mynet_ethtool_ops = {
    .get_link       = ethtool_op_get_link,
    .get_drvinfo    = mynet_get_drvinfo,
};
```

## probe 函数

```c
static int mynet_probe(struct platform_device *pdev)
{
    struct net_device *dev;
    struct mynet_priv *priv;
    int ret;

    /* 分配 net_device */
    dev = alloc_etherdev(sizeof(*priv));
    if (!dev)
        return -ENOMEM;

    priv = netdev_priv(dev);
    priv->netdev = dev;
    priv->dev = &pdev->dev;

    /* 设置 MAC 地址 */
    eth_hw_addr_set(dev, "\x00\x11\x22\x33\x44\x55");

    /* 设置 net_device_ops */
    dev->netdev_ops = &mynet_netdev_ops;
    dev->ethtool_ops = &mynet_ethtool_ops;
    dev->watchdog_timeo = msecs_to_jiffies(5000);

    /* NAPI 初始化 */
    netif_napi_add(dev, &priv->napi, mynet_poll, 64);

    /* 注册网络设备 */
    ret = register_netdev(dev);
    if (ret) {
        dev_err(&pdev->dev, "register_netdev failed\n");
        goto err_free;
    }

    platform_set_drvdata(pdev, dev);
    dev_info(&pdev->dev, "probe ok\n");
    return 0;

err_free:
    free_netdev(dev);
    return ret;
}
```

## open / stop

```c
static int mynet_open(struct net_device *dev)
{
    struct mynet_priv *priv = netdev_priv(dev);

    /* 申请中断 */
    ret = request_irq(priv->irq, mynet_isr, IRQF_SHARED, dev->name, dev);
    if (ret)
        return ret;

    /* 硬件初始化 */
    mynet_hw_init(priv);

    /* 启用 NAPI */
    napi_enable(&priv->napi);

    /* 告诉内核可以发送 */
    netif_start_queue(dev);

    return 0;
}

static int mynet_stop(struct net_device *dev)
{
    struct mynet_priv *priv = netdev_priv(dev);

    netif_stop_queue(dev);
    napi_disable(&priv->napi);
    free_irq(priv->irq, dev);
    mynet_hw_shutdown(priv);

    return 0;
}
```

## 发送（TX）

```c
static netdev_tx_t mynet_xmit(struct sk_buff *skb, struct net_device *dev)
{
    struct mynet_priv *priv = netdev_priv(dev);

    /* 检查队列是否满 */
    if (mynet_tx_full(priv)) {
        netif_stop_queue(dev);
        return NETDEV_TX_BUSY;
    }

    /* 将 skb 数据 DMA 到硬件 */
    dma_addr_t dma = dma_map_single(priv->dev, skb->data, skb->len,
                                     DMA_TO_DEVICE);
    if (dma_mapping_error(priv->dev, dma)) {
        dev_kfree_skb(skb);
        return NETDEV_TX_OK;
    }

    /* 编程硬件发送 */
    mynet_hw_xmit(priv, dma, skb->len);

    /* 记录时间戳（用于超时检测） */
    dev->trans_start = jiffies;

    /* 释放 skb（发送完成后由硬件中断释放，或在此释放） */
    dev_kfree_skb(skb);

    return NETDEV_TX_OK;
}
```

## 接收（RX）与 NAPI

```c
/* 硬中断：只确认中断，调度 NAPI */
static irqreturn_t mynet_isr(int irq, void *data)
{
    struct net_device *dev = data;
    struct mynet_priv *priv = netdev_priv(dev);

    u32 status = readl(priv->regs + IRQ_STATUS);
    if (!status)
        return IRQ_NONE;

    /* 禁用 RX 中断 */
    writel(0, priv->regs + IRQ_ENABLE);

    /* 调度 NAPI */
    napi_schedule(&priv->napi);

    return IRQ_HANDLED;
}

/* NAPI 轮询：在软中断上下文中处理收包 */
static int mynet_poll(struct napi_struct *napi, int budget)
{
    struct mynet_priv *priv = container_of(napi, struct mynet_priv, napi);
    struct net_device *dev = priv->netdev;
    int rx_count = 0;

    while (rx_count < budget) {
        struct sk_buff *skb;
        u32 len;

        /* 从硬件读取一个包 */
        len = mynet_hw_rx(priv, &skb);
        if (!len)
            break;

        skb->protocol = eth_type_trans(skb, dev);
        skb->ip_summed = CHECKSUM_UNNECESSARY;  /* 硬件校验 */

        /* 送入协议栈 */
        napi_gro_receive(napi, skb);
        rx_count++;
    }

    /* 如果处理完所有包，退出 NAPI */
    if (rx_count < budget) {
        napi_complete(napi);
        /* 重新使能 RX 中断 */
        writel(IRQ_RX_MASK, priv->regs + IRQ_ENABLE);
    }

    return rx_count;
}
```

## 统计信息

```c
static struct net_device_stats *mynet_get_stats(struct net_device *dev)
{
    struct mynet_priv *priv = netdev_priv(dev);
    struct net_device_stats *stats = &dev->stats;

    /* 从硬件读取统计 */
    stats->rx_packets = readl(priv->regs + RX_PKT_CNT);
    stats->tx_packets = readl(priv->regs + TX_PKT_CNT);
    stats->rx_bytes   = readl(priv->regs + RX_BYTE_CNT);
    stats->tx_bytes   = readl(priv->regs + TX_BYTE_CNT);
    stats->rx_errors  = readl(priv->regs + RX_ERR_CNT);
    stats->tx_errors  = readl(priv->regs + TX_ERR_CNT);
    stats->rx_dropped = readl(priv->regs + RX_DROP_CNT);

    return stats;
}
```

## ethtool 支持

```c
static void mynet_get_drvinfo(struct net_device *dev,
                               struct ethtool_drvinfo *info)
{
    strscpy(info->driver, DRV_NAME, sizeof(info->driver));
    strscpy(info->version, "1.0", sizeof(info->version));
}

/* 查看链路状态 */
static int mynet_get_link(struct net_device *dev)
{
    struct mynet_priv *priv = netdev_priv(dev);
    return (readl(priv->regs + LINK_STATUS) & LINK_UP) ? 1 : 0;
}
```

## 清理

```c
static int mynet_remove(struct platform_device *pdev)
{
    struct net_device *dev = platform_get_drvdata(pdev);
    struct mynet_priv *priv = netdev_priv(dev);

    unregister_netdev(dev);
    netif_napi_del(&priv->napi);
    free_netdev(dev);

    return 0;
}
```

## 网络命名空间支持

```c
/* 如需支持网络命名空间 */
static struct pernet_operations mynet_net_ops = {
    .init = mynet_ns_init,
    .exit = mynet_ns_exit,
    .id   = &mynet_net_id,
    .size = sizeof(struct mynet_net),
};

register_pernet_device(&mynet_net_ops);
```

## 调试

```bash
# 查看网络设备
ip link show
ifconfig -a

# 查看统计
ethtool -S eth0
cat /proc/net/dev

# 抓包
tcpdump -i eth0

# 查看 NAPI 状态
cat /proc/net/softnet_stat
```

## 常见陷阱

1. **alloc_etherdev 已分配私有数据** — 不要再 kzalloc
2. **register_netdev 可能失败** — 必须检查返回值
3. **skb 在 xmit 中由驱动接管** — 成功返回 NETDEV_TX_OK 后不要再次 free
4. **NAPI budget 必须返回实际处理数** — 不是请求的 budget
5. **napi_complete 必须在 napi_enable 之后调用**
6. **netif_stop_queue 后必须在适当时候 netif_wake_queue** — 否则发送永久阻塞
7. **DMA 映射必须在访问前完成** — dma_map_single / dma_unmap_single
8. **ethtool 是可选的但推荐实现** — 用户空间工具依赖它
