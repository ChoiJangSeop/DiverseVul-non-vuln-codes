static int probe_rio(struct usb_interface *intf,
		     const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct rio_usb_data *rio = &rio_instance;
	int retval;

	dev_info(&intf->dev, "USB Rio found at address %d\n", dev->devnum);

	retval = usb_register_dev(intf, &usb_rio_class);
	if (retval) {
		dev_err(&dev->dev,
			"Not able to get a minor for this device.\n");
		return -ENOMEM;
	}

	rio->rio_dev = dev;

	if (!(rio->obuf = kmalloc(OBUF_SIZE, GFP_KERNEL))) {
		dev_err(&dev->dev,
			"probe_rio: Not enough memory for the output buffer\n");
		usb_deregister_dev(intf, &usb_rio_class);
		return -ENOMEM;
	}
	dev_dbg(&intf->dev, "obuf address:%p\n", rio->obuf);

	if (!(rio->ibuf = kmalloc(IBUF_SIZE, GFP_KERNEL))) {
		dev_err(&dev->dev,
			"probe_rio: Not enough memory for the input buffer\n");
		usb_deregister_dev(intf, &usb_rio_class);
		kfree(rio->obuf);
		return -ENOMEM;
	}
	dev_dbg(&intf->dev, "ibuf address:%p\n", rio->ibuf);

	mutex_init(&(rio->lock));

	usb_set_intfdata (intf, rio);
	rio->present = 1;

	return 0;
}