tcf_fill_node(struct sk_buff *skb, struct tcf_proto *tp, unsigned long fh,
	      u32 pid, u32 seq, u16 flags, int event)
{
	struct tcmsg *tcm;
	struct nlmsghdr  *nlh;
	unsigned char	 *b = skb->tail;

	nlh = NLMSG_NEW(skb, pid, seq, event, sizeof(*tcm), flags);
	tcm = NLMSG_DATA(nlh);
	tcm->tcm_family = AF_UNSPEC;
	tcm->tcm_ifindex = tp->q->dev->ifindex;
	tcm->tcm_parent = tp->classid;
	tcm->tcm_info = TC_H_MAKE(tp->prio, tp->protocol);
	RTA_PUT(skb, TCA_KIND, IFNAMSIZ, tp->ops->kind);
	tcm->tcm_handle = fh;
	if (RTM_DELTFILTER != event) {
		tcm->tcm_handle = 0;
		if (tp->ops->dump && tp->ops->dump(tp, fh, skb, tcm) < 0)
			goto rtattr_failure;
	}
	nlh->nlmsg_len = skb->tail - b;
	return skb->len;

nlmsg_failure:
rtattr_failure:
	skb_trim(skb, b - skb->data);
	return -1;
}