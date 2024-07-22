static int caif_seqpkt_recvmsg(struct kiocb *iocb, struct socket *sock,
				struct msghdr *m, size_t len, int flags)

{
	struct sock *sk = sock->sk;
	struct sk_buff *skb;
	int ret;
	int copylen;

	ret = -EOPNOTSUPP;
	if (m->msg_flags&MSG_OOB)
		goto read_error;

	skb = skb_recv_datagram(sk, flags, 0 , &ret);
	if (!skb)
		goto read_error;
	copylen = skb->len;
	if (len < copylen) {
		m->msg_flags |= MSG_TRUNC;
		copylen = len;
	}

	ret = skb_copy_datagram_iovec(skb, 0, m->msg_iov, copylen);
	if (ret)
		goto out_free;

	ret = (flags & MSG_TRUNC) ? skb->len : copylen;
out_free:
	skb_free_datagram(sk, skb);
	caif_check_flow_release(sk);
	return ret;

read_error:
	return ret;
}