static int __ip6_datagram_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len)
{
	struct sockaddr_in6	*usin = (struct sockaddr_in6 *) uaddr;
	struct inet_sock	*inet = inet_sk(sk);
	struct ipv6_pinfo	*np = inet6_sk(sk);
	struct in6_addr	*daddr, *final_p, final;
	struct dst_entry	*dst;
	struct flowi6		fl6;
	struct ip6_flowlabel	*flowlabel = NULL;
	struct ipv6_txoptions	*opt;
	int			addr_type;
	int			err;

	if (usin->sin6_family == AF_INET) {
		if (__ipv6_only_sock(sk))
			return -EAFNOSUPPORT;
		err = __ip4_datagram_connect(sk, uaddr, addr_len);
		goto ipv4_connected;
	}

	if (addr_len < SIN6_LEN_RFC2133)
		return -EINVAL;

	if (usin->sin6_family != AF_INET6)
		return -EAFNOSUPPORT;

	memset(&fl6, 0, sizeof(fl6));
	if (np->sndflow) {
		fl6.flowlabel = usin->sin6_flowinfo&IPV6_FLOWINFO_MASK;
		if (fl6.flowlabel&IPV6_FLOWLABEL_MASK) {
			flowlabel = fl6_sock_lookup(sk, fl6.flowlabel);
			if (!flowlabel)
				return -EINVAL;
		}
	}

	addr_type = ipv6_addr_type(&usin->sin6_addr);

	if (addr_type == IPV6_ADDR_ANY) {
		/*
		 *	connect to self
		 */
		usin->sin6_addr.s6_addr[15] = 0x01;
	}

	daddr = &usin->sin6_addr;

	if (addr_type == IPV6_ADDR_MAPPED) {
		struct sockaddr_in sin;

		if (__ipv6_only_sock(sk)) {
			err = -ENETUNREACH;
			goto out;
		}
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = daddr->s6_addr32[3];
		sin.sin_port = usin->sin6_port;

		err = __ip4_datagram_connect(sk,
					     (struct sockaddr *) &sin,
					     sizeof(sin));

ipv4_connected:
		if (err)
			goto out;

		ipv6_addr_set_v4mapped(inet->inet_daddr, &sk->sk_v6_daddr);

		if (ipv6_addr_any(&np->saddr) ||
		    ipv6_mapped_addr_any(&np->saddr))
			ipv6_addr_set_v4mapped(inet->inet_saddr, &np->saddr);

		if (ipv6_addr_any(&sk->sk_v6_rcv_saddr) ||
		    ipv6_mapped_addr_any(&sk->sk_v6_rcv_saddr)) {
			ipv6_addr_set_v4mapped(inet->inet_rcv_saddr,
					       &sk->sk_v6_rcv_saddr);
			if (sk->sk_prot->rehash)
				sk->sk_prot->rehash(sk);
		}

		goto out;
	}

	if (__ipv6_addr_needs_scope_id(addr_type)) {
		if (addr_len >= sizeof(struct sockaddr_in6) &&
		    usin->sin6_scope_id) {
			if (sk->sk_bound_dev_if &&
			    sk->sk_bound_dev_if != usin->sin6_scope_id) {
				err = -EINVAL;
				goto out;
			}
			sk->sk_bound_dev_if = usin->sin6_scope_id;
		}

		if (!sk->sk_bound_dev_if && (addr_type & IPV6_ADDR_MULTICAST))
			sk->sk_bound_dev_if = np->mcast_oif;

		/* Connect to link-local address requires an interface */
		if (!sk->sk_bound_dev_if) {
			err = -EINVAL;
			goto out;
		}
	}

	sk->sk_v6_daddr = *daddr;
	np->flow_label = fl6.flowlabel;

	inet->inet_dport = usin->sin6_port;

	/*
	 *	Check for a route to destination an obtain the
	 *	destination cache for it.
	 */

	fl6.flowi6_proto = sk->sk_protocol;
	fl6.daddr = sk->sk_v6_daddr;
	fl6.saddr = np->saddr;
	fl6.flowi6_oif = sk->sk_bound_dev_if;
	fl6.flowi6_mark = sk->sk_mark;
	fl6.fl6_dport = inet->inet_dport;
	fl6.fl6_sport = inet->inet_sport;

	if (!fl6.flowi6_oif && (addr_type&IPV6_ADDR_MULTICAST))
		fl6.flowi6_oif = np->mcast_oif;

	security_sk_classify_flow(sk, flowi6_to_flowi(&fl6));

	opt = flowlabel ? flowlabel->opt : np->opt;
	final_p = fl6_update_dst(&fl6, opt, &final);

	dst = ip6_dst_lookup_flow(sk, &fl6, final_p);
	err = 0;
	if (IS_ERR(dst)) {
		err = PTR_ERR(dst);
		goto out;
	}

	/* source address lookup done in ip6_dst_lookup */

	if (ipv6_addr_any(&np->saddr))
		np->saddr = fl6.saddr;

	if (ipv6_addr_any(&sk->sk_v6_rcv_saddr)) {
		sk->sk_v6_rcv_saddr = fl6.saddr;
		inet->inet_rcv_saddr = LOOPBACK4_IPV6;
		if (sk->sk_prot->rehash)
			sk->sk_prot->rehash(sk);
	}

	ip6_dst_store(sk, dst,
		      ipv6_addr_equal(&fl6.daddr, &sk->sk_v6_daddr) ?
		      &sk->sk_v6_daddr : NULL,
#ifdef CONFIG_IPV6_SUBTREES
		      ipv6_addr_equal(&fl6.saddr, &np->saddr) ?
		      &np->saddr :
#endif
		      NULL);

	sk->sk_state = TCP_ESTABLISHED;
	sk_set_txhash(sk);
out:
	fl6_sock_release(flowlabel);
	return err;
}