static int inet_sk_reselect_saddr(struct sock *sk)
{
	struct inet_sock *inet = inet_sk(sk);
	__be32 old_saddr = inet->inet_saddr;
	__be32 daddr = inet->inet_daddr;
	struct flowi4 fl4;
	struct rtable *rt;
	__be32 new_saddr;

	if (inet->opt && inet->opt->srr)
		daddr = inet->opt->faddr;

	/* Query new route. */
	rt = ip_route_connect(&fl4, daddr, 0, RT_CONN_FLAGS(sk),
			      sk->sk_bound_dev_if, sk->sk_protocol,
			      inet->inet_sport, inet->inet_dport, sk, false);
	if (IS_ERR(rt))
		return PTR_ERR(rt);

	sk_setup_caps(sk, &rt->dst);

	new_saddr = rt->rt_src;

	if (new_saddr == old_saddr)
		return 0;

	if (sysctl_ip_dynaddr > 1) {
		printk(KERN_INFO "%s(): shifting inet->saddr from %pI4 to %pI4\n",
		       __func__, &old_saddr, &new_saddr);
	}

	inet->inet_saddr = inet->inet_rcv_saddr = new_saddr;

	/*
	 * XXX The only one ugly spot where we need to
	 * XXX really change the sockets identity after
	 * XXX it has entered the hashes. -DaveM
	 *
	 * Besides that, it does not check for connection
	 * uniqueness. Wait for troubles.
	 */
	__sk_prot_rehash(sk);
	return 0;
}