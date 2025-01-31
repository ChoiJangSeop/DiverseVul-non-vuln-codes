static void scsi_write_data(SCSIRequest *req)
{
    SCSIDiskReq *r = DO_UPCAST(SCSIDiskReq, req, req);
    SCSIDiskState *s = DO_UPCAST(SCSIDiskState, qdev, r->req.dev);
    uint32_t n;

    /* No data transfer may already be in progress */
    assert(r->req.aiocb == NULL);

    if (r->req.cmd.mode != SCSI_XFER_TO_DEV) {
        DPRINTF("Data transfer direction invalid\n");
        scsi_write_complete(r, -EINVAL);
        return;
    }

    n = r->iov.iov_len / 512;
    if (n) {
        if (s->tray_open) {
            scsi_write_complete(r, -ENOMEDIUM);
        }
        qemu_iovec_init_external(&r->qiov, &r->iov, 1);

        bdrv_acct_start(s->bs, &r->acct, n * BDRV_SECTOR_SIZE, BDRV_ACCT_WRITE);
        r->req.aiocb = bdrv_aio_writev(s->bs, r->sector, &r->qiov, n,
                                   scsi_write_complete, r);
        if (r->req.aiocb == NULL) {
            scsi_write_complete(r, -ENOMEM);
        }
    } else {
        /* Invoke completion routine to fetch data from host.  */
        scsi_write_complete(r, 0);
    }
}