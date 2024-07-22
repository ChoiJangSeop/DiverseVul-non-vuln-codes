__xfs_get_blocks(
	struct inode		*inode,
	sector_t		iblock,
	struct buffer_head	*bh_result,
	int			create,
	bool			direct,
	bool			dax_fault)
{
	struct xfs_inode	*ip = XFS_I(inode);
	struct xfs_mount	*mp = ip->i_mount;
	xfs_fileoff_t		offset_fsb, end_fsb;
	int			error = 0;
	int			lockmode = 0;
	struct xfs_bmbt_irec	imap;
	int			nimaps = 1;
	xfs_off_t		offset;
	ssize_t			size;
	int			new = 0;
	bool			is_cow = false;
	bool			need_alloc = false;

	BUG_ON(create && !direct);

	if (XFS_FORCED_SHUTDOWN(mp))
		return -EIO;

	offset = (xfs_off_t)iblock << inode->i_blkbits;
	ASSERT(bh_result->b_size >= (1 << inode->i_blkbits));
	size = bh_result->b_size;

	if (!create && offset >= i_size_read(inode))
		return 0;

	/*
	 * Direct I/O is usually done on preallocated files, so try getting
	 * a block mapping without an exclusive lock first.
	 */
	lockmode = xfs_ilock_data_map_shared(ip);

	ASSERT(offset <= mp->m_super->s_maxbytes);
	if (offset + size > mp->m_super->s_maxbytes)
		size = mp->m_super->s_maxbytes - offset;
	end_fsb = XFS_B_TO_FSB(mp, (xfs_ufsize_t)offset + size);
	offset_fsb = XFS_B_TO_FSBT(mp, offset);

	if (create && direct && xfs_is_reflink_inode(ip))
		is_cow = xfs_reflink_find_cow_mapping(ip, offset, &imap,
					&need_alloc);
	if (!is_cow) {
		error = xfs_bmapi_read(ip, offset_fsb, end_fsb - offset_fsb,
					&imap, &nimaps, XFS_BMAPI_ENTIRE);
		/*
		 * Truncate an overwrite extent if there's a pending CoW
		 * reservation before the end of this extent.  This
		 * forces us to come back to get_blocks to take care of
		 * the CoW.
		 */
		if (create && direct && nimaps &&
		    imap.br_startblock != HOLESTARTBLOCK &&
		    imap.br_startblock != DELAYSTARTBLOCK &&
		    !ISUNWRITTEN(&imap))
			xfs_reflink_trim_irec_to_next_cow(ip, offset_fsb,
					&imap);
	}
	ASSERT(!need_alloc);
	if (error)
		goto out_unlock;

	/* for DAX, we convert unwritten extents directly */
	if (create &&
	    (!nimaps ||
	     (imap.br_startblock == HOLESTARTBLOCK ||
	      imap.br_startblock == DELAYSTARTBLOCK) ||
	     (IS_DAX(inode) && ISUNWRITTEN(&imap)))) {
		/*
		 * xfs_iomap_write_direct() expects the shared lock. It
		 * is unlocked on return.
		 */
		if (lockmode == XFS_ILOCK_EXCL)
			xfs_ilock_demote(ip, lockmode);

		error = xfs_iomap_write_direct(ip, offset, size,
					       &imap, nimaps);
		if (error)
			return error;
		new = 1;

		trace_xfs_get_blocks_alloc(ip, offset, size,
				ISUNWRITTEN(&imap) ? XFS_IO_UNWRITTEN
						   : XFS_IO_DELALLOC, &imap);
	} else if (nimaps) {
		trace_xfs_get_blocks_found(ip, offset, size,
				ISUNWRITTEN(&imap) ? XFS_IO_UNWRITTEN
						   : XFS_IO_OVERWRITE, &imap);
		xfs_iunlock(ip, lockmode);
	} else {
		trace_xfs_get_blocks_notfound(ip, offset, size);
		goto out_unlock;
	}

	if (IS_DAX(inode) && create) {
		ASSERT(!ISUNWRITTEN(&imap));
		/* zeroing is not needed at a higher layer */
		new = 0;
	}

	/* trim mapping down to size requested */
	xfs_map_trim_size(inode, iblock, bh_result, &imap, offset, size);

	/*
	 * For unwritten extents do not report a disk address in the buffered
	 * read case (treat as if we're reading into a hole).
	 */
	if (imap.br_startblock != HOLESTARTBLOCK &&
	    imap.br_startblock != DELAYSTARTBLOCK &&
	    (create || !ISUNWRITTEN(&imap))) {
		if (create && direct && !is_cow) {
			error = xfs_bounce_unaligned_dio_write(ip, offset_fsb,
					&imap);
			if (error)
				return error;
		}

		xfs_map_buffer(inode, bh_result, &imap, offset);
		if (ISUNWRITTEN(&imap))
			set_buffer_unwritten(bh_result);
		/* direct IO needs special help */
		if (create) {
			if (dax_fault)
				ASSERT(!ISUNWRITTEN(&imap));
			else
				xfs_map_direct(inode, bh_result, &imap, offset,
						is_cow);
		}
	}

	/*
	 * If this is a realtime file, data may be on a different device.
	 * to that pointed to from the buffer_head b_bdev currently.
	 */
	bh_result->b_bdev = xfs_find_bdev_for_inode(inode);

	/*
	 * If we previously allocated a block out beyond eof and we are now
	 * coming back to use it then we will need to flag it as new even if it
	 * has a disk address.
	 *
	 * With sub-block writes into unwritten extents we also need to mark
	 * the buffer as new so that the unwritten parts of the buffer gets
	 * correctly zeroed.
	 */
	if (create &&
	    ((!buffer_mapped(bh_result) && !buffer_uptodate(bh_result)) ||
	     (offset >= i_size_read(inode)) ||
	     (new || ISUNWRITTEN(&imap))))
		set_buffer_new(bh_result);

	BUG_ON(direct && imap.br_startblock == DELAYSTARTBLOCK);

	return 0;

out_unlock:
	xfs_iunlock(ip, lockmode);
	return error;
}