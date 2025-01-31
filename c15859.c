void xenvif_disconnect(struct xenvif *vif)
{
	struct net_device *dev = vif->dev;
	if (netif_carrier_ok(dev)) {
		rtnl_lock();
		netif_carrier_off(dev); /* discard queued packets */
		if (netif_running(dev))
			xenvif_down(vif);
		rtnl_unlock();
		xenvif_put(vif);
	}

	atomic_dec(&vif->refcnt);
	wait_event(vif->waiting_to_free, atomic_read(&vif->refcnt) == 0);

	del_timer_sync(&vif->credit_timeout);

	if (vif->irq)
		unbind_from_irqhandler(vif->irq, vif);

	unregister_netdev(vif->dev);

	xen_netbk_unmap_frontend_rings(vif);

	free_netdev(vif->dev);
}