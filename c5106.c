static int cxusb_ctrl_msg(struct dvb_usb_device *d,
			  u8 cmd, u8 *wbuf, int wlen, u8 *rbuf, int rlen)
{
	struct cxusb_state *st = d->priv;
	int ret, wo;

	if (1 + wlen > MAX_XFER_SIZE) {
		warn("i2c wr: len=%d is too big!\n", wlen);
		return -EOPNOTSUPP;
	}

	wo = (rbuf == NULL || rlen == 0); /* write-only */

	mutex_lock(&d->data_mutex);
	st->data[0] = cmd;
	memcpy(&st->data[1], wbuf, wlen);
	if (wo)
		ret = dvb_usb_generic_write(d, st->data, 1 + wlen);
	else
		ret = dvb_usb_generic_rw(d, st->data, 1 + wlen,
					 rbuf, rlen, 0);

	mutex_unlock(&d->data_mutex);
	return ret;
}