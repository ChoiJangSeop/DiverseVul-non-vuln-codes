static int clie_5_attach(struct usb_serial *serial)
{
	struct usb_serial_port *port;
	unsigned int pipe;
	int j;

	/* TH55 registers 2 ports.
	   Communication in from the UX50/TH55 uses bulk_in_endpointAddress
	   from port 0. Communication out to the UX50/TH55 uses
	   bulk_out_endpointAddress from port 1

	   Lets do a quick and dirty mapping
	 */

	/* some sanity check */
	if (serial->num_ports < 2)
		return -1;

	/* port 0 now uses the modified endpoint Address */
	port = serial->port[0];
	port->bulk_out_endpointAddress =
				serial->port[1]->bulk_out_endpointAddress;

	pipe = usb_sndbulkpipe(serial->dev, port->bulk_out_endpointAddress);
	for (j = 0; j < ARRAY_SIZE(port->write_urbs); ++j)
		port->write_urbs[j]->pipe = pipe;

	return 0;
}