static int v9fs_xattr_set_acl(const struct xattr_handler *handler,
			      struct dentry *dentry, struct inode *inode,
			      const char *name, const void *value,
			      size_t size, int flags)
{
	int retval;
	struct posix_acl *acl;
	struct v9fs_session_info *v9ses;

	v9ses = v9fs_dentry2v9ses(dentry);
	/*
	 * set the attribute on the remote. Without even looking at the
	 * xattr value. We leave it to the server to validate
	 */
	if ((v9ses->flags & V9FS_ACCESS_MASK) != V9FS_ACCESS_CLIENT)
		return v9fs_xattr_set(dentry, handler->name, value, size,
				      flags);

	if (S_ISLNK(inode->i_mode))
		return -EOPNOTSUPP;
	if (!inode_owner_or_capable(inode))
		return -EPERM;
	if (value) {
		/* update the cached acl value */
		acl = posix_acl_from_xattr(&init_user_ns, value, size);
		if (IS_ERR(acl))
			return PTR_ERR(acl);
		else if (acl) {
			retval = posix_acl_valid(inode->i_sb->s_user_ns, acl);
			if (retval)
				goto err_out;
		}
	} else
		acl = NULL;

	switch (handler->flags) {
	case ACL_TYPE_ACCESS:
		if (acl) {
			umode_t mode = inode->i_mode;
			retval = posix_acl_equiv_mode(acl, &mode);
			if (retval < 0)
				goto err_out;
			else {
				struct iattr iattr;
				if (retval == 0) {
					/*
					 * ACL can be represented
					 * by the mode bits. So don't
					 * update ACL.
					 */
					acl = NULL;
					value = NULL;
					size = 0;
				}
				/* Updte the mode bits */
				iattr.ia_mode = ((mode & S_IALLUGO) |
						 (inode->i_mode & ~S_IALLUGO));
				iattr.ia_valid = ATTR_MODE;
				/* FIXME should we update ctime ?
				 * What is the following setxattr update the
				 * mode ?
				 */
				v9fs_vfs_setattr_dotl(dentry, &iattr);
			}
		}
		break;
	case ACL_TYPE_DEFAULT:
		if (!S_ISDIR(inode->i_mode)) {
			retval = acl ? -EINVAL : 0;
			goto err_out;
		}
		break;
	default:
		BUG();
	}
	retval = v9fs_xattr_set(dentry, handler->name, value, size, flags);
	if (!retval)
		set_cached_acl(inode, handler->flags, acl);
err_out:
	posix_acl_release(acl);
	return retval;
}