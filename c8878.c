mptctl_readtest (unsigned long arg)
{
	struct mpt_ioctl_test __user *uarg = (void __user *) arg;
	struct mpt_ioctl_test	 karg;
	MPT_ADAPTER *ioc;
	int iocnum;

	if (copy_from_user(&karg, uarg, sizeof(struct mpt_ioctl_test))) {
		printk(KERN_ERR MYNAM "%s@%d::mptctl_readtest - "
			"Unable to read in mpt_ioctl_test struct @ %p\n",
				__FILE__, __LINE__, uarg);
		return -EFAULT;
	}

	if (((iocnum = mpt_verify_adapter(karg.hdr.iocnum, &ioc)) < 0) ||
	    (ioc == NULL)) {
		printk(KERN_DEBUG MYNAM "%s::mptctl_readtest() @%d - ioc%d not found!\n",
				__FILE__, __LINE__, iocnum);
		return -ENODEV;
	}

	dctlprintk(ioc, printk(MYIOC_s_DEBUG_FMT "mptctl_readtest called.\n",
	    ioc->name));
	/* Fill in the data and return the structure to the calling
	 * program
	 */

#ifdef MFCNT
	karg.chip_type = ioc->mfcnt;
#else
	karg.chip_type = ioc->pcidev->device;
#endif
	strcpy_s(karg.name, MPT_MAX_NAME, ioc->name);
	karg.name[MPT_MAX_NAME-1]='\0';
	strcpy_s(karg.product, MPT_PRODUCT_LENGTH, ioc->prod_name);
	karg.product[MPT_PRODUCT_LENGTH-1]='\0';

	/* Copy the data from kernel memory to user memory
	 */
	if (copy_to_user((char __user *)arg, &karg, sizeof(struct mpt_ioctl_test))) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_readtest - "
			"Unable to write out mpt_ioctl_test struct @ %p\n",
			ioc->name, __FILE__, __LINE__, uarg);
		return -EFAULT;
	}

	return 0;
}