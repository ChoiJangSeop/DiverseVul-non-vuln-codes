static ssize_t aio_setup_vectored_rw(struct kiocb *kiocb,
				     int rw, char __user *buf,
				     unsigned long *nr_segs,
				     size_t *len,
				     struct iovec **iovec,
				     bool compat)
{
	ssize_t ret;

	*nr_segs = *len;

#ifdef CONFIG_COMPAT
	if (compat)
		ret = compat_rw_copy_check_uvector(rw,
				(struct compat_iovec __user *)buf,
				*nr_segs, UIO_FASTIOV, *iovec, iovec);
	else
#endif
		ret = rw_copy_check_uvector(rw,
				(struct iovec __user *)buf,
				*nr_segs, UIO_FASTIOV, *iovec, iovec);
	if (ret < 0)
		return ret;

	/* len now reflect bytes instead of segs */
	*len = ret;
	return 0;
}