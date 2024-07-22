static void pipe_advance(struct iov_iter *i, size_t size)
{
	struct pipe_inode_info *pipe = i->pipe;
	struct pipe_buffer *buf;
	int idx = i->idx;
	size_t off = i->iov_offset, orig_sz;
	
	if (unlikely(i->count < size))
		size = i->count;
	orig_sz = size;

	if (size) {
		if (off) /* make it relative to the beginning of buffer */
			size += off - pipe->bufs[idx].offset;
		while (1) {
			buf = &pipe->bufs[idx];
			if (size <= buf->len)
				break;
			size -= buf->len;
			idx = next_idx(idx, pipe);
		}
		buf->len = size;
		i->idx = idx;
		off = i->iov_offset = buf->offset + size;
	}
	if (off)
		idx = next_idx(idx, pipe);
	if (pipe->nrbufs) {
		int unused = (pipe->curbuf + pipe->nrbufs) & (pipe->buffers - 1);
		/* [curbuf,unused) is in use.  Free [idx,unused) */
		while (idx != unused) {
			pipe_buf_release(pipe, &pipe->bufs[idx]);
			idx = next_idx(idx, pipe);
			pipe->nrbufs--;
		}
	}
	i->count -= orig_sz;
}