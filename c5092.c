static int catc_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct device *dev = &intf->dev;
	struct usb_device *usbdev = interface_to_usbdev(intf);
	struct net_device *netdev;
	struct catc *catc;
	u8 broadcast[ETH_ALEN];
	int i, pktsz, ret;

	if (usb_set_interface(usbdev,
			intf->altsetting->desc.bInterfaceNumber, 1)) {
		dev_err(dev, "Can't set altsetting 1.\n");
		return -EIO;
	}

	netdev = alloc_etherdev(sizeof(struct catc));
	if (!netdev)
		return -ENOMEM;

	catc = netdev_priv(netdev);

	netdev->netdev_ops = &catc_netdev_ops;
	netdev->watchdog_timeo = TX_TIMEOUT;
	netdev->ethtool_ops = &ops;

	catc->usbdev = usbdev;
	catc->netdev = netdev;

	spin_lock_init(&catc->tx_lock);
	spin_lock_init(&catc->ctrl_lock);

	init_timer(&catc->timer);
	catc->timer.data = (long) catc;
	catc->timer.function = catc_stats_timer;

	catc->ctrl_urb = usb_alloc_urb(0, GFP_KERNEL);
	catc->tx_urb = usb_alloc_urb(0, GFP_KERNEL);
	catc->rx_urb = usb_alloc_urb(0, GFP_KERNEL);
	catc->irq_urb = usb_alloc_urb(0, GFP_KERNEL);
	if ((!catc->ctrl_urb) || (!catc->tx_urb) || 
	    (!catc->rx_urb) || (!catc->irq_urb)) {
		dev_err(&intf->dev, "No free urbs available.\n");
		ret = -ENOMEM;
		goto fail_free;
	}

	/* The F5U011 has the same vendor/product as the netmate but a device version of 0x130 */
	if (le16_to_cpu(usbdev->descriptor.idVendor) == 0x0423 && 
	    le16_to_cpu(usbdev->descriptor.idProduct) == 0xa &&
	    le16_to_cpu(catc->usbdev->descriptor.bcdDevice) == 0x0130) {
		dev_dbg(dev, "Testing for f5u011\n");
		catc->is_f5u011 = 1;		
		atomic_set(&catc->recq_sz, 0);
		pktsz = RX_PKT_SZ;
	} else {
		pktsz = RX_MAX_BURST * (PKT_SZ + 2);
	}
	
	usb_fill_control_urb(catc->ctrl_urb, usbdev, usb_sndctrlpipe(usbdev, 0),
		NULL, NULL, 0, catc_ctrl_done, catc);

	usb_fill_bulk_urb(catc->tx_urb, usbdev, usb_sndbulkpipe(usbdev, 1),
		NULL, 0, catc_tx_done, catc);

	usb_fill_bulk_urb(catc->rx_urb, usbdev, usb_rcvbulkpipe(usbdev, 1),
		catc->rx_buf, pktsz, catc_rx_done, catc);

	usb_fill_int_urb(catc->irq_urb, usbdev, usb_rcvintpipe(usbdev, 2),
                catc->irq_buf, 2, catc_irq_done, catc, 1);

	if (!catc->is_f5u011) {
		dev_dbg(dev, "Checking memory size\n");

		i = 0x12345678;
		catc_write_mem(catc, 0x7a80, &i, 4);
		i = 0x87654321;	
		catc_write_mem(catc, 0xfa80, &i, 4);
		catc_read_mem(catc, 0x7a80, &i, 4);
	  
		switch (i) {
		case 0x12345678:
			catc_set_reg(catc, TxBufCount, 8);
			catc_set_reg(catc, RxBufCount, 32);
			dev_dbg(dev, "64k Memory\n");
			break;
		default:
			dev_warn(&intf->dev,
				 "Couldn't detect memory size, assuming 32k\n");
		case 0x87654321:
			catc_set_reg(catc, TxBufCount, 4);
			catc_set_reg(catc, RxBufCount, 16);
			dev_dbg(dev, "32k Memory\n");
			break;
		}
	  
		dev_dbg(dev, "Getting MAC from SEEROM.\n");
	  
		catc_get_mac(catc, netdev->dev_addr);
		
		dev_dbg(dev, "Setting MAC into registers.\n");
	  
		for (i = 0; i < 6; i++)
			catc_set_reg(catc, StationAddr0 - i, netdev->dev_addr[i]);
		
		dev_dbg(dev, "Filling the multicast list.\n");
	  
		eth_broadcast_addr(broadcast);
		catc_multicast(broadcast, catc->multicast);
		catc_multicast(netdev->dev_addr, catc->multicast);
		catc_write_mem(catc, 0xfa80, catc->multicast, 64);
		
		dev_dbg(dev, "Clearing error counters.\n");
		
		for (i = 0; i < 8; i++)
			catc_set_reg(catc, EthStats + i, 0);
		catc->last_stats = jiffies;
		
		dev_dbg(dev, "Enabling.\n");
		
		catc_set_reg(catc, MaxBurst, RX_MAX_BURST);
		catc_set_reg(catc, OpModes, OpTxMerge | OpRxMerge | OpLenInclude | Op3MemWaits);
		catc_set_reg(catc, LEDCtrl, LEDLink);
		catc_set_reg(catc, RxUnit, RxEnable | RxPolarity | RxMultiCast);
	} else {
		dev_dbg(dev, "Performing reset\n");
		catc_reset(catc);
		catc_get_mac(catc, netdev->dev_addr);
		
		dev_dbg(dev, "Setting RX Mode\n");
		catc->rxmode[0] = RxEnable | RxPolarity | RxMultiCast;
		catc->rxmode[1] = 0;
		f5u011_rxmode(catc, catc->rxmode);
	}
	dev_dbg(dev, "Init done.\n");
	printk(KERN_INFO "%s: %s USB Ethernet at usb-%s-%s, %pM.\n",
	       netdev->name, (catc->is_f5u011) ? "Belkin F5U011" : "CATC EL1210A NetMate",
	       usbdev->bus->bus_name, usbdev->devpath, netdev->dev_addr);
	usb_set_intfdata(intf, catc);

	SET_NETDEV_DEV(netdev, &intf->dev);
	ret = register_netdev(netdev);
	if (ret)
		goto fail_clear_intfdata;

	return 0;

fail_clear_intfdata:
	usb_set_intfdata(intf, NULL);
fail_free:
	usb_free_urb(catc->ctrl_urb);
	usb_free_urb(catc->tx_urb);
	usb_free_urb(catc->rx_urb);
	usb_free_urb(catc->irq_urb);
	free_netdev(netdev);
	return ret;
}