int ar6000_create_ap_interface(struct ar6_softc *ar, char *ap_ifname)
{
    struct net_device *dev;
    struct ar_virtual_interface *arApDev;

    dev = alloc_etherdev(sizeof(struct ar_virtual_interface));
    if (dev == NULL) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_create_ap_interface: can't alloc etherdev\n"));
        return A_ERROR;
    } 
    
    ether_setup(dev);
    init_netdev(dev, ap_ifname);

    if (register_netdev(dev)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_create_ap_interface: register_netdev failed\n"));
        return A_ERROR;
    }

    arApDev = netdev_priv(dev);
    arApDev->arDev = ar;
    arApDev->arNetDev = dev;
    arApDev->arStaNetDev = ar->arNetDev;

    ar->arApDev = arApDev;
    arApNetDev = dev;

    /* Copy the MAC address */
    memcpy(dev->dev_addr, ar->arNetDev->dev_addr, AR6000_ETH_ADDR_LEN);

    return 0;
}