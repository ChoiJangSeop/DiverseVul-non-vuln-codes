tca_get_fill(struct sk_buff *skb, struct tc_action *a, u32 pid, u32 seq,
             u16 flags, int event, int bind, int ref)
{
	struct tcamsg *t;
	struct nlmsghdr *nlh;
	unsigned char *b = skb->tail;
	struct rtattr *x;

	nlh = NLMSG_NEW(skb, pid, seq, event, sizeof(*t), flags);

	t = NLMSG_DATA(nlh);
	t->tca_family = AF_UNSPEC;
	
	x = (struct rtattr*) skb->tail;
	RTA_PUT(skb, TCA_ACT_TAB, 0, NULL);

	if (tcf_action_dump(skb, a, bind, ref) < 0)
		goto rtattr_failure;

	x->rta_len = skb->tail - (u8*)x;
	
	nlh->nlmsg_len = skb->tail - b;
	return skb->len;

rtattr_failure:
nlmsg_failure:
	skb_trim(skb, b - skb->data);
	return -1;
}