static int dn_create(struct net *net, struct socket *sock, int protocol,
		     int kern)
{
	struct sock *sk;

	if (!net_eq(net, &init_net))
		return -EAFNOSUPPORT;

	switch (sock->type) {
	case SOCK_SEQPACKET:
		if (protocol != DNPROTO_NSP)
			return -EPROTONOSUPPORT;
		break;
	case SOCK_STREAM:
		break;
	default:
		return -ESOCKTNOSUPPORT;
	}


	if ((sk = dn_alloc_sock(net, sock, GFP_KERNEL, kern)) == NULL)
		return -ENOBUFS;

	sk->sk_protocol = protocol;

	return 0;
}