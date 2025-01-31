static int fr_add_pvc(struct net_device *frad, unsigned int dlci, int type)
{
	hdlc_device *hdlc = dev_to_hdlc(frad);
	pvc_device *pvc;
	struct net_device *dev;
	int used;

	if ((pvc = add_pvc(frad, dlci)) == NULL) {
		netdev_warn(frad, "Memory squeeze on fr_add_pvc()\n");
		return -ENOBUFS;
	}

	if (*get_dev_p(pvc, type))
		return -EEXIST;

	used = pvc_is_used(pvc);

	if (type == ARPHRD_ETHER)
		dev = alloc_netdev(0, "pvceth%d", ether_setup);
	else
		dev = alloc_netdev(0, "pvc%d", pvc_setup);

	if (!dev) {
		netdev_warn(frad, "Memory squeeze on fr_pvc()\n");
		delete_unused_pvcs(hdlc);
		return -ENOBUFS;
	}

	if (type == ARPHRD_ETHER)
		random_ether_addr(dev->dev_addr);
	else {
		*(__be16*)dev->dev_addr = htons(dlci);
		dlci_to_q922(dev->broadcast, dlci);
	}
	dev->netdev_ops = &pvc_ops;
	dev->mtu = HDLC_MAX_MTU;
	dev->tx_queue_len = 0;
	dev->ml_priv = pvc;

	if (register_netdevice(dev) != 0) {
		free_netdev(dev);
		delete_unused_pvcs(hdlc);
		return -EIO;
	}

	dev->destructor = free_netdev;
	*get_dev_p(pvc, type) = dev;
	if (!used) {
		state(hdlc)->dce_changed = 1;
		state(hdlc)->dce_pvc_count++;
	}
	return 0;
}