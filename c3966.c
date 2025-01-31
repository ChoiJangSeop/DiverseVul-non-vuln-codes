nfsd4_set_nfs4_acl(struct svc_rqst *rqstp, struct svc_fh *fhp,
		struct nfs4_acl *acl)
{
	__be32 error;
	int host_error;
	struct dentry *dentry;
	struct inode *inode;
	struct posix_acl *pacl = NULL, *dpacl = NULL;
	unsigned int flags = 0;

	/* Get inode */
	error = fh_verify(rqstp, fhp, 0, NFSD_MAY_SATTR);
	if (error)
		return error;

	dentry = fhp->fh_dentry;
	inode = d_inode(dentry);

	if (!inode->i_op->set_acl || !IS_POSIXACL(inode))
		return nfserr_attrnotsupp;

	if (S_ISDIR(inode->i_mode))
		flags = NFS4_ACL_DIR;

	host_error = nfs4_acl_nfsv4_to_posix(acl, &pacl, &dpacl, flags);
	if (host_error == -EINVAL)
		return nfserr_attrnotsupp;
	if (host_error < 0)
		goto out_nfserr;

	host_error = inode->i_op->set_acl(inode, pacl, ACL_TYPE_ACCESS);
	if (host_error < 0)
		goto out_release;

	if (S_ISDIR(inode->i_mode)) {
		host_error = inode->i_op->set_acl(inode, dpacl,
						  ACL_TYPE_DEFAULT);
	}

out_release:
	posix_acl_release(pacl);
	posix_acl_release(dpacl);
out_nfserr:
	if (host_error == -EOPNOTSUPP)
		return nfserr_attrnotsupp;
	else
		return nfserrno(host_error);
}