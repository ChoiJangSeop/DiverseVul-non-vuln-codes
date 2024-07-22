void ip_send_reply(struct sock *sk, struct sk_buff *skb, struct ip_reply_arg *arg,
		   unsigned int len)
{
	struct inet_sock *inet = inet_sk(sk);
	struct {
		struct ip_options	opt;
		char			data[40];
	} replyopts;
	struct ipcm_cookie ipc;
	__be32 daddr;
	struct rtable *rt = skb_rtable(skb);

	if (ip_options_echo(&replyopts.opt, skb))
		return;

	daddr = ipc.addr = rt->rt_src;
	ipc.opt = NULL;
	ipc.tx_flags = 0;

	if (replyopts.opt.optlen) {
		ipc.opt = &replyopts.opt;

		if (ipc.opt->srr)
			daddr = replyopts.opt.faddr;
	}

	{
		struct flowi4 fl4;

		flowi4_init_output(&fl4, arg->bound_dev_if, 0,
				   RT_TOS(ip_hdr(skb)->tos),
				   RT_SCOPE_UNIVERSE, sk->sk_protocol,
				   ip_reply_arg_flowi_flags(arg),
				   daddr, rt->rt_spec_dst,
				   tcp_hdr(skb)->source, tcp_hdr(skb)->dest);
		security_skb_classify_flow(skb, flowi4_to_flowi(&fl4));
		rt = ip_route_output_key(sock_net(sk), &fl4);
		if (IS_ERR(rt))
			return;
	}

	/* And let IP do all the hard work.

	   This chunk is not reenterable, hence spinlock.
	   Note that it uses the fact, that this function is called
	   with locally disabled BH and that sk cannot be already spinlocked.
	 */
	bh_lock_sock(sk);
	inet->tos = ip_hdr(skb)->tos;
	sk->sk_priority = skb->priority;
	sk->sk_protocol = ip_hdr(skb)->protocol;
	sk->sk_bound_dev_if = arg->bound_dev_if;
	ip_append_data(sk, ip_reply_glue_bits, arg->iov->iov_base, len, 0,
		       &ipc, &rt, MSG_DONTWAIT);
	if ((skb = skb_peek(&sk->sk_write_queue)) != NULL) {
		if (arg->csumoffset >= 0)
			*((__sum16 *)skb_transport_header(skb) +
			  arg->csumoffset) = csum_fold(csum_add(skb->csum,
								arg->csum));
		skb->ip_summed = CHECKSUM_NONE;
		ip_push_pending_frames(sk);
	}

	bh_unlock_sock(sk);

	ip_rt_put(rt);
}