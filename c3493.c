int iwch_cxgb3_ofld_send(struct t3cdev *tdev, struct sk_buff *skb)
{
	int	error = 0;
	struct cxio_rdev *rdev;

	rdev = (struct cxio_rdev *)tdev->ulp;
	if (cxio_fatal_error(rdev)) {
		kfree_skb(skb);
		return -EIO;
	}
	error = cxgb3_ofld_send(tdev, skb);
	if (error < 0)
		kfree_skb(skb);
	return error;
}