static int wanxl_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	const size_t size = sizeof(sync_serial_settings);
	sync_serial_settings line;
	port_t *port = dev_to_port(dev);

	if (cmd != SIOCWANDEV)
		return hdlc_ioctl(dev, ifr, cmd);

	switch (ifr->ifr_settings.type) {
	case IF_GET_IFACE:
		ifr->ifr_settings.type = IF_IFACE_SYNC_SERIAL;
		if (ifr->ifr_settings.size < size) {
			ifr->ifr_settings.size = size; /* data size wanted */
			return -ENOBUFS;
		}
		line.clock_type = get_status(port)->clocking;
		line.clock_rate = 0;
		line.loopback = 0;

		if (copy_to_user(ifr->ifr_settings.ifs_ifsu.sync, &line, size))
			return -EFAULT;
		return 0;

	case IF_IFACE_SYNC_SERIAL:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		if (dev->flags & IFF_UP)
			return -EBUSY;

		if (copy_from_user(&line, ifr->ifr_settings.ifs_ifsu.sync,
				   size))
			return -EFAULT;

		if (line.clock_type != CLOCK_EXT &&
		    line.clock_type != CLOCK_TXFROMRX)
			return -EINVAL; /* No such clock setting */

		if (line.loopback != 0)
			return -EINVAL;

		get_status(port)->clocking = line.clock_type;
		return 0;

	default:
		return hdlc_ioctl(dev, ifr, cmd);
        }
}