nfs3svc_decode_writeargs(struct svc_rqst *rqstp, __be32 *p,
					struct nfsd3_writeargs *args)
{
	unsigned int len, v, hdr, dlen;
	u32 max_blocksize = svc_max_payload(rqstp);
	struct kvec *head = rqstp->rq_arg.head;
	struct kvec *tail = rqstp->rq_arg.tail;

	p = decode_fh(p, &args->fh);
	if (!p)
		return 0;
	p = xdr_decode_hyper(p, &args->offset);

	args->count = ntohl(*p++);
	args->stable = ntohl(*p++);
	len = args->len = ntohl(*p++);
	/*
	 * The count must equal the amount of data passed.
	 */
	if (args->count != args->len)
		return 0;

	/*
	 * Check to make sure that we got the right number of
	 * bytes.
	 */
	hdr = (void*)p - head->iov_base;
	dlen = head->iov_len + rqstp->rq_arg.page_len + tail->iov_len - hdr;
	/*
	 * Round the length of the data which was specified up to
	 * the next multiple of XDR units and then compare that
	 * against the length which was actually received.
	 * Note that when RPCSEC/GSS (for example) is used, the
	 * data buffer can be padded so dlen might be larger
	 * than required.  It must never be smaller.
	 */
	if (dlen < XDR_QUADLEN(len)*4)
		return 0;

	if (args->count > max_blocksize) {
		args->count = max_blocksize;
		len = args->len = max_blocksize;
	}
	rqstp->rq_vec[0].iov_base = (void*)p;
	rqstp->rq_vec[0].iov_len = head->iov_len - hdr;
	v = 0;
	while (len > rqstp->rq_vec[v].iov_len) {
		len -= rqstp->rq_vec[v].iov_len;
		v++;
		rqstp->rq_vec[v].iov_base = page_address(rqstp->rq_pages[v]);
		rqstp->rq_vec[v].iov_len = PAGE_SIZE;
	}
	rqstp->rq_vec[v].iov_len = len;
	args->vlen = v + 1;
	return 1;
}