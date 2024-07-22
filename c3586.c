static int digi_startup(struct usb_serial *serial)
{
	struct digi_serial *serial_priv;
	int ret;

	serial_priv = kzalloc(sizeof(*serial_priv), GFP_KERNEL);
	if (!serial_priv)
		return -ENOMEM;

	spin_lock_init(&serial_priv->ds_serial_lock);
	serial_priv->ds_oob_port_num = serial->type->num_ports;
	serial_priv->ds_oob_port = serial->port[serial_priv->ds_oob_port_num];

	ret = digi_port_init(serial_priv->ds_oob_port,
						serial_priv->ds_oob_port_num);
	if (ret) {
		kfree(serial_priv);
		return ret;
	}

	usb_set_serial_data(serial, serial_priv);

	return 0;
}