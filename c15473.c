isofs_export_encode_fh(struct inode *inode,
		       __u32 *fh32,
		       int *max_len,
		       struct inode *parent)
{
	struct iso_inode_info * ei = ISOFS_I(inode);
	int len = *max_len;
	int type = 1;
	__u16 *fh16 = (__u16*)fh32;

	/*
	 * WARNING: max_len is 5 for NFSv2.  Because of this
	 * limitation, we use the lower 16 bits of fh32[1] to hold the
	 * offset of the inode and the upper 16 bits of fh32[1] to
	 * hold the offset of the parent.
	 */
	if (parent && (len < 5)) {
		*max_len = 5;
		return 255;
	} else if (len < 3) {
		*max_len = 3;
		return 255;
	}

	len = 3;
	fh32[0] = ei->i_iget5_block;
 	fh16[2] = (__u16)ei->i_iget5_offset;  /* fh16 [sic] */
	fh32[2] = inode->i_generation;
	if (parent) {
		struct iso_inode_info *eparent;
		eparent = ISOFS_I(parent);
		fh32[3] = eparent->i_iget5_block;
		fh16[3] = (__u16)eparent->i_iget5_offset;  /* fh16 [sic] */
		fh32[4] = parent->i_generation;
		len = 5;
		type = 2;
	}
	*max_len = len;
	return type;
}