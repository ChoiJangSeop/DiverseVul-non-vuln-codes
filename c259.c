static int blk_fill_sgv4_hdr_rq(struct request_queue *q, struct request *rq,
				struct sg_io_v4 *hdr, struct bsg_device *bd,
				fmode_t has_write_perm)
{
	if (hdr->request_len > BLK_MAX_CDB) {
		rq->cmd = kzalloc(hdr->request_len, GFP_KERNEL);
		if (!rq->cmd)
			return -ENOMEM;
	}

	if (copy_from_user(rq->cmd, (void *)(unsigned long)hdr->request,
			   hdr->request_len))
		return -EFAULT;

	if (hdr->subprotocol == BSG_SUB_PROTOCOL_SCSI_CMD) {
		if (blk_verify_command(&q->cmd_filter, rq->cmd, has_write_perm))
			return -EPERM;
	} else if (!capable(CAP_SYS_RAWIO))
		return -EPERM;

	/*
	 * fill in request structure
	 */
	rq->cmd_len = hdr->request_len;
	rq->cmd_type = REQ_TYPE_BLOCK_PC;

	rq->timeout = (hdr->timeout * HZ) / 1000;
	if (!rq->timeout)
		rq->timeout = q->sg_timeout;
	if (!rq->timeout)
		rq->timeout = BLK_DEFAULT_SG_TIMEOUT;

	return 0;
}