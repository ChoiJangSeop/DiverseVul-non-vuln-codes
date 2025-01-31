int ip_options_get_from_user(struct net *net, struct ip_options **optp,
			     unsigned char __user *data, int optlen)
{
	struct ip_options *opt = ip_options_get_alloc(optlen);

	if (!opt)
		return -ENOMEM;
	if (optlen && copy_from_user(opt->__data, data, optlen)) {
		kfree(opt);
		return -EFAULT;
	}
	return ip_options_get_finish(net, optp, opt, optlen);
}