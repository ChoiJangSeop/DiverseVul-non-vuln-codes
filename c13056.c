void bnep_net_setup(struct net_device *dev)
{

	memset(dev->broadcast, 0xff, ETH_ALEN);
	dev->addr_len = ETH_ALEN;

	ether_setup(dev);
	dev->netdev_ops = &bnep_netdev_ops;

	dev->watchdog_timeo  = HZ * 2;
}