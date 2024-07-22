static u64 __skb_get_nlattr_nest(u64 ctx, u64 A, u64 X, u64 r4, u64 r5)
{
	struct sk_buff *skb = (struct sk_buff *)(long) ctx;
	struct nlattr *nla;

	if (skb_is_nonlinear(skb))
		return 0;

	if (A > skb->len - sizeof(struct nlattr))
		return 0;

	nla = (struct nlattr *) &skb->data[A];
	if (nla->nla_len > A - skb->len)
		return 0;

	nla = nla_find_nested(nla, X);
	if (nla)
		return (void *) nla - (void *) skb->data;

	return 0;
}