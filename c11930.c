static void v9fs_read(void *opaque)
{
    int32_t fid;
    uint64_t off;
    ssize_t err = 0;
    int32_t count = 0;
    size_t offset = 7;
    uint32_t max_count;
    V9fsFidState *fidp;
    V9fsPDU *pdu = opaque;
    V9fsState *s = pdu->s;

    err = pdu_unmarshal(pdu, offset, "dqd", &fid, &off, &max_count);
    if (err < 0) {
        goto out_nofid;
    }
    trace_v9fs_read(pdu->tag, pdu->id, fid, off, max_count);

    fidp = get_fid(pdu, fid);
    if (fidp == NULL) {
        err = -EINVAL;
        goto out_nofid;
    }
    if (fidp->fid_type == P9_FID_DIR) {

        if (off == 0) {
            v9fs_co_rewinddir(pdu, fidp);
        }
        count = v9fs_do_readdir_with_stat(pdu, fidp, max_count);
        if (count < 0) {
            err = count;
            goto out;
        }
        err = pdu_marshal(pdu, offset, "d", count);
        if (err < 0) {
            goto out;
        }
        err += offset + count;
    } else if (fidp->fid_type == P9_FID_FILE) {
        QEMUIOVector qiov_full;
        QEMUIOVector qiov;
        int32_t len;

        v9fs_init_qiov_from_pdu(&qiov_full, pdu, offset + 4, max_count, false);
        qemu_iovec_init(&qiov, qiov_full.niov);
        do {
            qemu_iovec_reset(&qiov);
            qemu_iovec_concat(&qiov, &qiov_full, count, qiov_full.size - count);
            if (0) {
                print_sg(qiov.iov, qiov.niov);
            }
            /* Loop in case of EINTR */
            do {
                len = v9fs_co_preadv(pdu, fidp, qiov.iov, qiov.niov, off);
                if (len >= 0) {
                    off   += len;
                    count += len;
                }
            } while (len == -EINTR && !pdu->cancelled);
            if (len < 0) {
                /* IO error return the error */
                err = len;
                goto out;
            }
        } while (count < max_count && len > 0);
        err = pdu_marshal(pdu, offset, "d", count);
        if (err < 0) {
            goto out;
        }
        err += offset + count;
        qemu_iovec_destroy(&qiov);
        qemu_iovec_destroy(&qiov_full);
    } else if (fidp->fid_type == P9_FID_XATTR) {
        err = v9fs_xattr_read(s, pdu, fidp, off, max_count);
    } else {
        err = -EINVAL;
    }
    trace_v9fs_read_return(pdu->tag, pdu->id, count, err);
out:
    put_fid(pdu, fidp);
out_nofid:
    pdu_complete(pdu, err);
}