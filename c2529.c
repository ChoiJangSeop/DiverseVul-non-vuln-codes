pipe_read(struct kiocb *iocb, const struct iovec *_iov,
	   unsigned long nr_segs, loff_t pos)
{
	struct file *filp = iocb->ki_filp;
	struct pipe_inode_info *pipe = filp->private_data;
	int do_wakeup;
	ssize_t ret;
	struct iovec *iov = (struct iovec *)_iov;
	size_t total_len;

	total_len = iov_length(iov, nr_segs);
	/* Null read succeeds. */
	if (unlikely(total_len == 0))
		return 0;

	do_wakeup = 0;
	ret = 0;
	__pipe_lock(pipe);
	for (;;) {
		int bufs = pipe->nrbufs;
		if (bufs) {
			int curbuf = pipe->curbuf;
			struct pipe_buffer *buf = pipe->bufs + curbuf;
			const struct pipe_buf_operations *ops = buf->ops;
			void *addr;
			size_t chars = buf->len;
			int error, atomic;

			if (chars > total_len)
				chars = total_len;

			error = ops->confirm(pipe, buf);
			if (error) {
				if (!ret)
					ret = error;
				break;
			}

			atomic = !iov_fault_in_pages_write(iov, chars);
redo:
			if (atomic)
				addr = kmap_atomic(buf->page);
			else
				addr = kmap(buf->page);
			error = pipe_iov_copy_to_user(iov, addr + buf->offset, chars, atomic);
			if (atomic)
				kunmap_atomic(addr);
			else
				kunmap(buf->page);
			if (unlikely(error)) {
				/*
				 * Just retry with the slow path if we failed.
				 */
				if (atomic) {
					atomic = 0;
					goto redo;
				}
				if (!ret)
					ret = error;
				break;
			}
			ret += chars;
			buf->offset += chars;
			buf->len -= chars;

			/* Was it a packet buffer? Clean up and exit */
			if (buf->flags & PIPE_BUF_FLAG_PACKET) {
				total_len = chars;
				buf->len = 0;
			}

			if (!buf->len) {
				buf->ops = NULL;
				ops->release(pipe, buf);
				curbuf = (curbuf + 1) & (pipe->buffers - 1);
				pipe->curbuf = curbuf;
				pipe->nrbufs = --bufs;
				do_wakeup = 1;
			}
			total_len -= chars;
			if (!total_len)
				break;	/* common path: read succeeded */
		}
		if (bufs)	/* More to do? */
			continue;
		if (!pipe->writers)
			break;
		if (!pipe->waiting_writers) {
			/* syscall merging: Usually we must not sleep
			 * if O_NONBLOCK is set, or if we got some data.
			 * But if a writer sleeps in kernel space, then
			 * we can wait for that data without violating POSIX.
			 */
			if (ret)
				break;
			if (filp->f_flags & O_NONBLOCK) {
				ret = -EAGAIN;
				break;
			}
		}
		if (signal_pending(current)) {
			if (!ret)
				ret = -ERESTARTSYS;
			break;
		}
		if (do_wakeup) {
			wake_up_interruptible_sync_poll(&pipe->wait, POLLOUT | POLLWRNORM);
 			kill_fasync(&pipe->fasync_writers, SIGIO, POLL_OUT);
		}
		pipe_wait(pipe);
	}
	__pipe_unlock(pipe);

	/* Signal writers asynchronously that there is more room. */
	if (do_wakeup) {
		wake_up_interruptible_sync_poll(&pipe->wait, POLLOUT | POLLWRNORM);
		kill_fasync(&pipe->fasync_writers, SIGIO, POLL_OUT);
	}
	if (ret > 0)
		file_accessed(filp);
	return ret;
}