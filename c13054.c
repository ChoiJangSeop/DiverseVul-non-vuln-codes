void vlan_setup(struct net_device *dev)
{
	ether_setup(dev);

	dev->priv_flags		|= IFF_802_1Q_VLAN;
	dev->priv_flags		&= ~IFF_XMIT_DST_RELEASE;
	dev->tx_queue_len	= 0;

	dev->netdev_ops		= &vlan_netdev_ops;
	dev->destructor		= free_netdev;
	dev->ethtool_ops	= &vlan_ethtool_ops;

	memset(dev->broadcast, 0, ETH_ALEN);
}