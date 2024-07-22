void usb_deregister_dev(struct usb_interface *intf,
			struct usb_class_driver *class_driver)
{
	if (intf->minor == -1)
		return;

	dev_dbg(&intf->dev, "removing %d minor\n", intf->minor);

	down_write(&minor_rwsem);
	usb_minors[intf->minor] = NULL;
	up_write(&minor_rwsem);

	device_destroy(usb_class->class, MKDEV(USB_MAJOR, intf->minor));
	intf->usb_dev = NULL;
	intf->minor = -1;
	destroy_usb_class();
}