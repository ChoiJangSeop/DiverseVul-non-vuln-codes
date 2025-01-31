static int cdc_ncm_bind(struct usbnet *dev, struct usb_interface *intf)
{
	int ret;

	/* MBIM backwards compatible function? */
	if (cdc_ncm_select_altsetting(intf) != CDC_NCM_COMM_ALTSETTING_NCM)
		return -ENODEV;

	/* The NCM data altsetting is fixed, so we hard-coded it.
	 * Additionally, generic NCM devices are assumed to accept arbitrarily
	 * placed NDP.
	 */
	ret = cdc_ncm_bind_common(dev, intf, CDC_NCM_DATA_ALTSETTING_NCM, 0);

	/*
	 * We should get an event when network connection is "connected" or
	 * "disconnected". Set network connection in "disconnected" state
	 * (carrier is OFF) during attach, so the IP network stack does not
	 * start IPv6 negotiation and more.
	 */
	usbnet_link_change(dev, 0, 0);
	return ret;
}