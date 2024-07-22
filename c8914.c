static int stv06xx_isoc_nego(struct gspca_dev *gspca_dev)
{
	int ret, packet_size, min_packet_size;
	struct usb_host_interface *alt;
	struct sd *sd = (struct sd *) gspca_dev;

	alt = &gspca_dev->dev->actconfig->intf_cache[0]->altsetting[1];
	packet_size = le16_to_cpu(alt->endpoint[0].desc.wMaxPacketSize);
	min_packet_size = sd->sensor->min_packet_size[gspca_dev->curr_mode];
	if (packet_size <= min_packet_size)
		return -EIO;

	packet_size -= 100;
	if (packet_size < min_packet_size)
		packet_size = min_packet_size;
	alt->endpoint[0].desc.wMaxPacketSize = cpu_to_le16(packet_size);

	ret = usb_set_interface(gspca_dev->dev, gspca_dev->iface, 1);
	if (ret < 0)
		gspca_err(gspca_dev, "set alt 1 err %d\n", ret);

	return ret;
}