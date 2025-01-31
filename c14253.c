static ssize_t __nfs4_get_acl_uncached(struct inode *inode, void *buf, size_t buflen)
{
	struct page *pages[NFS4ACL_MAXPAGES];
	struct nfs_getaclargs args = {
		.fh = NFS_FH(inode),
		.acl_pages = pages,
		.acl_len = buflen,
	};
	struct nfs_getaclres res = {
		.acl_len = buflen,
	};
	void *resp_buf;
	struct rpc_message msg = {
		.rpc_proc = &nfs4_procedures[NFSPROC4_CLNT_GETACL],
		.rpc_argp = &args,
		.rpc_resp = &res,
	};
	struct page *localpage = NULL;
	int ret;

	if (buflen < PAGE_SIZE) {
		/* As long as we're doing a round trip to the server anyway,
		 * let's be prepared for a page of acl data. */
		localpage = alloc_page(GFP_KERNEL);
		resp_buf = page_address(localpage);
		if (localpage == NULL)
			return -ENOMEM;
		args.acl_pages[0] = localpage;
		args.acl_pgbase = 0;
		args.acl_len = PAGE_SIZE;
	} else {
		resp_buf = buf;
		buf_to_pages(buf, buflen, args.acl_pages, &args.acl_pgbase);
	}
	ret = nfs4_call_sync(NFS_SERVER(inode)->client, NFS_SERVER(inode), &msg, &args.seq_args, &res.seq_res, 0);
	if (ret)
		goto out_free;
	if (res.acl_len > args.acl_len)
		nfs4_write_cached_acl(inode, NULL, res.acl_len);
	else
		nfs4_write_cached_acl(inode, resp_buf, res.acl_len);
	if (buf) {
		ret = -ERANGE;
		if (res.acl_len > buflen)
			goto out_free;
		if (localpage)
			memcpy(buf, resp_buf, res.acl_len);
	}
	ret = res.acl_len;
out_free:
	if (localpage)
		__free_page(localpage);
	return ret;
}