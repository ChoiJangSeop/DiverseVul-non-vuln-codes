static int tcf_add_notify(struct tc_action *a, u32 pid, u32 seq, int event,
                          u16 flags)
{
	struct tcamsg *t;
	struct nlmsghdr *nlh;
	struct sk_buff *skb;
	struct rtattr *x;
	unsigned char *b;
	int err = 0;

	skb = alloc_skb(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!skb)
		return -ENOBUFS;

	b = (unsigned char *)skb->tail;

	nlh = NLMSG_NEW(skb, pid, seq, event, sizeof(*t), flags);
	t = NLMSG_DATA(nlh);
	t->tca_family = AF_UNSPEC;
	
	x = (struct rtattr*) skb->tail;
	RTA_PUT(skb, TCA_ACT_TAB, 0, NULL);

	if (tcf_action_dump(skb, a, 0, 0) < 0)
		goto rtattr_failure;

	x->rta_len = skb->tail - (u8*)x;
	
	nlh->nlmsg_len = skb->tail - b;
	NETLINK_CB(skb).dst_groups = RTMGRP_TC;
	
	err = rtnetlink_send(skb, pid, RTMGRP_TC, flags&NLM_F_ECHO);
	if (err > 0)
		err = 0;
	return err;

rtattr_failure:
nlmsg_failure:
	skb_trim(skb, b - skb->data);
	return -1;
}