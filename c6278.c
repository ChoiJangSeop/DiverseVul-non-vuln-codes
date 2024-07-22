static int stub_probe(struct usb_device *udev)
{
	struct stub_device *sdev = NULL;
	const char *udev_busid = dev_name(&udev->dev);
	struct bus_id_priv *busid_priv;
	int rc;

	dev_dbg(&udev->dev, "Enter probe\n");

	/* check we should claim or not by busid_table */
	busid_priv = get_busid_priv(udev_busid);
	if (!busid_priv || (busid_priv->status == STUB_BUSID_REMOV) ||
	    (busid_priv->status == STUB_BUSID_OTHER)) {
		dev_info(&udev->dev,
			"%s is not in match_busid table... skip!\n",
			udev_busid);

		/*
		 * Return value should be ENODEV or ENOXIO to continue trying
		 * other matched drivers by the driver core.
		 * See driver_probe_device() in driver/base/dd.c
		 */
		return -ENODEV;
	}

	if (udev->descriptor.bDeviceClass == USB_CLASS_HUB) {
		dev_dbg(&udev->dev, "%s is a usb hub device... skip!\n",
			 udev_busid);
		return -ENODEV;
	}

	if (!strcmp(udev->bus->bus_name, "vhci_hcd")) {
		dev_dbg(&udev->dev,
			"%s is attached on vhci_hcd... skip!\n",
			udev_busid);

		return -ENODEV;
	}

	/* ok, this is my device */
	sdev = stub_device_alloc(udev);
	if (!sdev)
		return -ENOMEM;

	dev_info(&udev->dev,
		"usbip-host: register new device (bus %u dev %u)\n",
		udev->bus->busnum, udev->devnum);

	busid_priv->shutdown_busid = 0;

	/* set private data to usb_device */
	dev_set_drvdata(&udev->dev, sdev);
	busid_priv->sdev = sdev;
	busid_priv->udev = udev;

	/*
	 * Claim this hub port.
	 * It doesn't matter what value we pass as owner
	 * (struct dev_state) as long as it is unique.
	 */
	rc = usb_hub_claim_port(udev->parent, udev->portnum,
			(struct usb_dev_state *) udev);
	if (rc) {
		dev_dbg(&udev->dev, "unable to claim port\n");
		goto err_port;
	}

	rc = stub_add_files(&udev->dev);
	if (rc) {
		dev_err(&udev->dev, "stub_add_files for %s\n", udev_busid);
		goto err_files;
	}
	busid_priv->status = STUB_BUSID_ALLOC;

	return 0;
err_files:
	usb_hub_release_port(udev->parent, udev->portnum,
			     (struct usb_dev_state *) udev);
err_port:
	dev_set_drvdata(&udev->dev, NULL);
	usb_put_dev(udev);

	busid_priv->sdev = NULL;
	stub_device_free(sdev);
	return rc;
}