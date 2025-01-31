static int l2cap_build_conf_req(struct sock *sk, void *data)
{
	struct l2cap_pinfo *pi = l2cap_pi(sk);
	struct l2cap_conf_req *req = data;
	struct l2cap_conf_rfc rfc = { .mode = L2CAP_MODE_BASIC };
	void *ptr = req->data;

	BT_DBG("sk %p", sk);

	switch (pi->mode) {
	case L2CAP_MODE_BASIC:
		if (pi->imtu != L2CAP_DEFAULT_MTU)
			l2cap_add_conf_opt(&ptr, L2CAP_CONF_MTU, 2, pi->imtu);
		break;

	case L2CAP_MODE_ERTM:
		rfc.mode            = L2CAP_MODE_ERTM;
		rfc.txwin_size      = L2CAP_DEFAULT_RX_WINDOW;
		rfc.max_transmit    = L2CAP_DEFAULT_MAX_RECEIVE;
		rfc.retrans_timeout = cpu_to_le16(L2CAP_DEFAULT_RETRANS_TO);
		rfc.monitor_timeout = cpu_to_le16(L2CAP_DEFAULT_MONITOR_TO);
		rfc.max_pdu_size    = cpu_to_le16(L2CAP_DEFAULT_MAX_RX_APDU);

		l2cap_add_conf_opt(&ptr, L2CAP_CONF_RFC,
					sizeof(rfc), (unsigned long) &rfc);
		break;
	}

	/* FIXME: Need actual value of the flush timeout */
	//if (flush_to != L2CAP_DEFAULT_FLUSH_TO)
	//   l2cap_add_conf_opt(&ptr, L2CAP_CONF_FLUSH_TO, 2, pi->flush_to);

	req->dcid  = cpu_to_le16(pi->dcid);
	req->flags = cpu_to_le16(0);

	return ptr - data;
}