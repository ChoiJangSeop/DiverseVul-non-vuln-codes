static int fscrypt_d_revalidate(struct dentry *dentry, unsigned int flags)
{
	struct dentry *dir;
	struct fscrypt_info *ci;
	int dir_has_key, cached_with_key;

	if (flags & LOOKUP_RCU)
		return -ECHILD;

	dir = dget_parent(dentry);
	if (!d_inode(dir)->i_sb->s_cop->is_encrypted(d_inode(dir))) {
		dput(dir);
		return 0;
	}

	ci = d_inode(dir)->i_crypt_info;
	if (ci && ci->ci_keyring_key &&
	    (ci->ci_keyring_key->flags & ((1 << KEY_FLAG_INVALIDATED) |
					  (1 << KEY_FLAG_REVOKED) |
					  (1 << KEY_FLAG_DEAD))))
		ci = NULL;

	/* this should eventually be an flag in d_flags */
	spin_lock(&dentry->d_lock);
	cached_with_key = dentry->d_flags & DCACHE_ENCRYPTED_WITH_KEY;
	spin_unlock(&dentry->d_lock);
	dir_has_key = (ci != NULL);
	dput(dir);

	/*
	 * If the dentry was cached without the key, and it is a
	 * negative dentry, it might be a valid name.  We can't check
	 * if the key has since been made available due to locking
	 * reasons, so we fail the validation so ext4_lookup() can do
	 * this check.
	 *
	 * We also fail the validation if the dentry was created with
	 * the key present, but we no longer have the key, or vice versa.
	 */
	if ((!cached_with_key && d_is_negative(dentry)) ||
			(!cached_with_key && dir_has_key) ||
			(cached_with_key && !dir_has_key))
		return 0;
	return 1;
}