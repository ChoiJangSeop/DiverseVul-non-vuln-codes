compat_mpt_command(struct file *filp, unsigned int cmd,
			unsigned long arg)
{
	struct mpt_ioctl_command32 karg32;
	struct mpt_ioctl_command32 __user *uarg = (struct mpt_ioctl_command32 __user *) arg;
	struct mpt_ioctl_command karg;
	MPT_ADAPTER *iocp = NULL;
	int iocnum, iocnumX;
	int nonblock = (filp->f_flags & O_NONBLOCK);
	int ret;

	if (copy_from_user(&karg32, (char __user *)arg, sizeof(karg32)))
		return -EFAULT;

	/* Verify intended MPT adapter */
	iocnumX = karg32.hdr.iocnum & 0xFF;
	if (((iocnum = mpt_verify_adapter(iocnumX, &iocp)) < 0) ||
	    (iocp == NULL)) {
		printk(KERN_DEBUG MYNAM "::compat_mpt_command @%d - ioc%d not found!\n",
			__LINE__, iocnumX);
		return -ENODEV;
	}

	if ((ret = mptctl_syscall_down(iocp, nonblock)) != 0)
		return ret;

	dctlprintk(iocp, printk(MYIOC_s_DEBUG_FMT "compat_mpt_command() called\n",
	    iocp->name));
	/* Copy data to karg */
	karg.hdr.iocnum = karg32.hdr.iocnum;
	karg.hdr.port = karg32.hdr.port;
	karg.timeout = karg32.timeout;
	karg.maxReplyBytes = karg32.maxReplyBytes;

	karg.dataInSize = karg32.dataInSize;
	karg.dataOutSize = karg32.dataOutSize;
	karg.maxSenseBytes = karg32.maxSenseBytes;
	karg.dataSgeOffset = karg32.dataSgeOffset;

	karg.replyFrameBufPtr = (char __user *)(unsigned long)karg32.replyFrameBufPtr;
	karg.dataInBufPtr = (char __user *)(unsigned long)karg32.dataInBufPtr;
	karg.dataOutBufPtr = (char __user *)(unsigned long)karg32.dataOutBufPtr;
	karg.senseDataPtr = (char __user *)(unsigned long)karg32.senseDataPtr;

	/* Pass new structure to do_mpt_command
	 */
	ret = mptctl_do_mpt_command (karg, &uarg->MF);

	mutex_unlock(&iocp->ioctl_cmds.mutex);

	return ret;
}