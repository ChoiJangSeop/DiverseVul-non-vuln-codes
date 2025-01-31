bool inode_owner_or_capable(const struct inode *inode)
{
	if (uid_eq(current_fsuid(), inode->i_uid))
		return true;
	if (inode_capable(inode, CAP_FOWNER))
		return true;
	return false;
}