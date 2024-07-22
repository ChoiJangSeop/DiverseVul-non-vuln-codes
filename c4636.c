static void nfnetlink_rcv_batch(struct sk_buff *skb, struct nlmsghdr *nlh,
				u_int16_t subsys_id)
{
	struct sk_buff *oskb = skb;
	struct net *net = sock_net(skb->sk);
	const struct nfnetlink_subsystem *ss;
	const struct nfnl_callback *nc;
	static LIST_HEAD(err_list);
	u32 status;
	int err;

	if (subsys_id >= NFNL_SUBSYS_COUNT)
		return netlink_ack(skb, nlh, -EINVAL);
replay:
	status = 0;

	skb = netlink_skb_clone(oskb, GFP_KERNEL);
	if (!skb)
		return netlink_ack(oskb, nlh, -ENOMEM);

	nfnl_lock(subsys_id);
	ss = nfnl_dereference_protected(subsys_id);
	if (!ss) {
#ifdef CONFIG_MODULES
		nfnl_unlock(subsys_id);
		request_module("nfnetlink-subsys-%d", subsys_id);
		nfnl_lock(subsys_id);
		ss = nfnl_dereference_protected(subsys_id);
		if (!ss)
#endif
		{
			nfnl_unlock(subsys_id);
			netlink_ack(oskb, nlh, -EOPNOTSUPP);
			return kfree_skb(skb);
		}
	}

	if (!ss->commit || !ss->abort) {
		nfnl_unlock(subsys_id);
		netlink_ack(oskb, nlh, -EOPNOTSUPP);
		return kfree_skb(skb);
	}

	while (skb->len >= nlmsg_total_size(0)) {
		int msglen, type;

		nlh = nlmsg_hdr(skb);
		err = 0;

		if (nlmsg_len(nlh) < sizeof(struct nfgenmsg) ||
		    skb->len < nlh->nlmsg_len) {
			err = -EINVAL;
			goto ack;
		}

		/* Only requests are handled by the kernel */
		if (!(nlh->nlmsg_flags & NLM_F_REQUEST)) {
			err = -EINVAL;
			goto ack;
		}

		type = nlh->nlmsg_type;
		if (type == NFNL_MSG_BATCH_BEGIN) {
			/* Malformed: Batch begin twice */
			nfnl_err_reset(&err_list);
			status |= NFNL_BATCH_FAILURE;
			goto done;
		} else if (type == NFNL_MSG_BATCH_END) {
			status |= NFNL_BATCH_DONE;
			goto done;
		} else if (type < NLMSG_MIN_TYPE) {
			err = -EINVAL;
			goto ack;
		}

		/* We only accept a batch with messages for the same
		 * subsystem.
		 */
		if (NFNL_SUBSYS_ID(type) != subsys_id) {
			err = -EINVAL;
			goto ack;
		}

		nc = nfnetlink_find_client(type, ss);
		if (!nc) {
			err = -EINVAL;
			goto ack;
		}

		{
			int min_len = nlmsg_total_size(sizeof(struct nfgenmsg));
			u_int8_t cb_id = NFNL_MSG_TYPE(nlh->nlmsg_type);
			struct nlattr *cda[ss->cb[cb_id].attr_count + 1];
			struct nlattr *attr = (void *)nlh + min_len;
			int attrlen = nlh->nlmsg_len - min_len;

			err = nla_parse(cda, ss->cb[cb_id].attr_count,
					attr, attrlen, ss->cb[cb_id].policy);
			if (err < 0)
				goto ack;

			if (nc->call_batch) {
				err = nc->call_batch(net, net->nfnl, skb, nlh,
						     (const struct nlattr **)cda);
			}

			/* The lock was released to autoload some module, we
			 * have to abort and start from scratch using the
			 * original skb.
			 */
			if (err == -EAGAIN) {
				status |= NFNL_BATCH_REPLAY;
				goto next;
			}
		}
ack:
		if (nlh->nlmsg_flags & NLM_F_ACK || err) {
			/* Errors are delivered once the full batch has been
			 * processed, this avoids that the same error is
			 * reported several times when replaying the batch.
			 */
			if (nfnl_err_add(&err_list, nlh, err) < 0) {
				/* We failed to enqueue an error, reset the
				 * list of errors and send OOM to userspace
				 * pointing to the batch header.
				 */
				nfnl_err_reset(&err_list);
				netlink_ack(oskb, nlmsg_hdr(oskb), -ENOMEM);
				status |= NFNL_BATCH_FAILURE;
				goto done;
			}
			/* We don't stop processing the batch on errors, thus,
			 * userspace gets all the errors that the batch
			 * triggers.
			 */
			if (err)
				status |= NFNL_BATCH_FAILURE;
		}
next:
		msglen = NLMSG_ALIGN(nlh->nlmsg_len);
		if (msglen > skb->len)
			msglen = skb->len;
		skb_pull(skb, msglen);
	}
done:
	if (status & NFNL_BATCH_REPLAY) {
		ss->abort(net, oskb);
		nfnl_err_reset(&err_list);
		nfnl_unlock(subsys_id);
		kfree_skb(skb);
		goto replay;
	} else if (status == NFNL_BATCH_DONE) {
		ss->commit(net, oskb);
	} else {
		ss->abort(net, oskb);
	}

	nfnl_err_deliver(&err_list, oskb);
	nfnl_unlock(subsys_id);
	kfree_skb(skb);
}