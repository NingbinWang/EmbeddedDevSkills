/*
 * 网络设备驱动模板（纯软件回环）
 * 编译: make
 * 加载: sudo insmod net_device.ko
 * 测试: sudo ifconfig mynet 10.0.0.1 up && ping 10.0.0.1
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#define DRV_NAME "mynet"

struct mynet_priv {
    struct net_device *netdev;
    struct napi_struct napi;
    unsigned long tx_packets;
    unsigned long rx_packets;
};

static int mynet_open(struct net_device *dev)
{
    netif_start_queue(dev);
    return 0;
}

static int mynet_stop(struct net_device *dev)
{
    netif_stop_queue(dev);
    return 0;
}

/* 发送：直接回环到接收 */
static netdev_tx_t mynet_xmit(struct sk_buff *skb, struct net_device *dev)
{
    struct mynet_priv *priv = netdev_priv(dev);

    priv->tx_packets++;
    dev->stats.tx_bytes += skb->len;

    /* 回环：伪造接收 */
    skb->protocol = eth_type_trans(skb, dev);
    skb->ip_summed = CHECKSUM_UNNECESSARY;
    netif_rx(skb);

    priv->rx_packets++;
    dev->stats.rx_bytes += skb->len;

    return NETDEV_TX_OK;
}

static struct net_device_stats *mynet_get_stats(struct net_device *dev)
{
    struct mynet_priv *priv = netdev_priv(dev);
    dev->stats.tx_packets = priv->tx_packets;
    dev->stats.rx_packets = priv->rx_packets;
    return &dev->stats;
}

static const struct net_device_ops mynet_netdev_ops = {
    .ndo_open        = mynet_open,
    .ndo_stop        = mynet_stop,
    .ndo_start_xmit  = mynet_xmit,
    .ndo_get_stats   = mynet_get_stats,
    .ndo_set_rx_mode = mynet_set_rx_mode,
    .ndo_validate_addr = eth_validate_addr,
};

static void mynet_setup(struct net_device *dev)
{
    dev->netdev_ops = &mynet_netdev_ops;
    dev->flags |= IFF_NOARP;
    dev->features |= NETIF_F_HW_CSUM;
    ether_setup(dev);
}

static struct net_device *g_dev;

static int __init mynet_init(void)
{
    struct net_device *dev;
    struct mynet_priv *priv;
    int ret;

    dev = alloc_netdev(sizeof(*priv), DRV_NAME, NET_NAME_UNKNOWN, mynet_setup);
    if (!dev) return -ENOMEM;

    priv = netdev_priv(dev);
    priv->netdev = dev;

    eth_hw_addr_set(dev, "\x00\x11\x22\x33\x44\x55");

    ret = register_netdev(dev);
    if (ret) {
        free_netdev(dev);
        return ret;
    }

    g_dev = dev;
    pr_info(DRV_NAME ": loaded\n");
    return 0;
}

static void __exit mynet_exit(void)
{
    unregister_netdev(g_dev);
    free_netdev(g_dev);
    pr_info(DRV_NAME ": unloaded\n");
}

module_init(mynet_init);
module_exit(mynet_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple network device driver template (loopback)");
