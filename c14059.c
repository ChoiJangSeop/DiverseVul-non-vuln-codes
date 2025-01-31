ext4_xattr_block_set(handle_t *handle, struct inode *inode,
		     struct ext4_xattr_info *i,
		     struct ext4_xattr_block_find *bs)
{
	struct super_block *sb = inode->i_sb;
	struct buffer_head *new_bh = NULL;
	struct ext4_xattr_search *s = &bs->s;
	struct mb_cache_entry *ce = NULL;
	int error = 0;
	struct mb_cache *ext4_mb_cache = EXT4_GET_MB_CACHE(inode);

#define header(x) ((struct ext4_xattr_header *)(x))

	if (i->value && i->value_len > sb->s_blocksize)
		return -ENOSPC;
	if (s->base) {
		ce = mb_cache_entry_get(ext4_mb_cache, bs->bh->b_bdev,
					bs->bh->b_blocknr);
		BUFFER_TRACE(bs->bh, "get_write_access");
		error = ext4_journal_get_write_access(handle, bs->bh);
		if (error)
			goto cleanup;
		lock_buffer(bs->bh);

		if (header(s->base)->h_refcount == cpu_to_le32(1)) {
			if (ce) {
				mb_cache_entry_free(ce);
				ce = NULL;
			}
			ea_bdebug(bs->bh, "modifying in-place");
			error = ext4_xattr_set_entry(i, s);
			if (!error) {
				if (!IS_LAST_ENTRY(s->first))
					ext4_xattr_rehash(header(s->base),
							  s->here);
				ext4_xattr_cache_insert(ext4_mb_cache,
					bs->bh);
			}
			unlock_buffer(bs->bh);
			if (error == -EFSCORRUPTED)
				goto bad_block;
			if (!error)
				error = ext4_handle_dirty_xattr_block(handle,
								      inode,
								      bs->bh);
			if (error)
				goto cleanup;
			goto inserted;
		} else {
			int offset = (char *)s->here - bs->bh->b_data;

			unlock_buffer(bs->bh);
			if (ce) {
				mb_cache_entry_release(ce);
				ce = NULL;
			}
			ea_bdebug(bs->bh, "cloning");
			s->base = kmalloc(bs->bh->b_size, GFP_NOFS);
			error = -ENOMEM;
			if (s->base == NULL)
				goto cleanup;
			memcpy(s->base, BHDR(bs->bh), bs->bh->b_size);
			s->first = ENTRY(header(s->base)+1);
			header(s->base)->h_refcount = cpu_to_le32(1);
			s->here = ENTRY(s->base + offset);
			s->end = s->base + bs->bh->b_size;
		}
	} else {
		/* Allocate a buffer where we construct the new block. */
		s->base = kzalloc(sb->s_blocksize, GFP_NOFS);
		/* assert(header == s->base) */
		error = -ENOMEM;
		if (s->base == NULL)
			goto cleanup;
		header(s->base)->h_magic = cpu_to_le32(EXT4_XATTR_MAGIC);
		header(s->base)->h_blocks = cpu_to_le32(1);
		header(s->base)->h_refcount = cpu_to_le32(1);
		s->first = ENTRY(header(s->base)+1);
		s->here = ENTRY(header(s->base)+1);
		s->end = s->base + sb->s_blocksize;
	}

	error = ext4_xattr_set_entry(i, s);
	if (error == -EFSCORRUPTED)
		goto bad_block;
	if (error)
		goto cleanup;
	if (!IS_LAST_ENTRY(s->first))
		ext4_xattr_rehash(header(s->base), s->here);

inserted:
	if (!IS_LAST_ENTRY(s->first)) {
		new_bh = ext4_xattr_cache_find(inode, header(s->base), &ce);
		if (new_bh) {
			/* We found an identical block in the cache. */
			if (new_bh == bs->bh)
				ea_bdebug(new_bh, "keeping");
			else {
				/* The old block is released after updating
				   the inode. */
				error = dquot_alloc_block(inode,
						EXT4_C2B(EXT4_SB(sb), 1));
				if (error)
					goto cleanup;
				BUFFER_TRACE(new_bh, "get_write_access");
				error = ext4_journal_get_write_access(handle,
								      new_bh);
				if (error)
					goto cleanup_dquot;
				lock_buffer(new_bh);
				le32_add_cpu(&BHDR(new_bh)->h_refcount, 1);
				ea_bdebug(new_bh, "reusing; refcount now=%d",
					le32_to_cpu(BHDR(new_bh)->h_refcount));
				unlock_buffer(new_bh);
				error = ext4_handle_dirty_xattr_block(handle,
								      inode,
								      new_bh);
				if (error)
					goto cleanup_dquot;
			}
			mb_cache_entry_release(ce);
			ce = NULL;
		} else if (bs->bh && s->base == bs->bh->b_data) {
			/* We were modifying this block in-place. */
			ea_bdebug(bs->bh, "keeping this block");
			new_bh = bs->bh;
			get_bh(new_bh);
		} else {
			/* We need to allocate a new block */
			ext4_fsblk_t goal, block;

			goal = ext4_group_first_block_no(sb,
						EXT4_I(inode)->i_block_group);

			/* non-extent files can't have physical blocks past 2^32 */
			if (!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS)))
				goal = goal & EXT4_MAX_BLOCK_FILE_PHYS;

			block = ext4_new_meta_blocks(handle, inode, goal, 0,
						     NULL, &error);
			if (error)
				goto cleanup;

			if (!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS)))
				BUG_ON(block > EXT4_MAX_BLOCK_FILE_PHYS);

			ea_idebug(inode, "creating block %llu",
				  (unsigned long long)block);

			new_bh = sb_getblk(sb, block);
			if (unlikely(!new_bh)) {
				error = -ENOMEM;
getblk_failed:
				ext4_free_blocks(handle, inode, NULL, block, 1,
						 EXT4_FREE_BLOCKS_METADATA);
				goto cleanup;
			}
			lock_buffer(new_bh);
			error = ext4_journal_get_create_access(handle, new_bh);
			if (error) {
				unlock_buffer(new_bh);
				error = -EIO;
				goto getblk_failed;
			}
			memcpy(new_bh->b_data, s->base, new_bh->b_size);
			set_buffer_uptodate(new_bh);
			unlock_buffer(new_bh);
			ext4_xattr_cache_insert(ext4_mb_cache, new_bh);
			error = ext4_handle_dirty_xattr_block(handle,
							      inode, new_bh);
			if (error)
				goto cleanup;
		}
	}

	/* Update the inode. */
	EXT4_I(inode)->i_file_acl = new_bh ? new_bh->b_blocknr : 0;

	/* Drop the previous xattr block. */
	if (bs->bh && bs->bh != new_bh)
		ext4_xattr_release_block(handle, inode, bs->bh);
	error = 0;

cleanup:
	if (ce)
		mb_cache_entry_release(ce);
	brelse(new_bh);
	if (!(bs->bh && s->base == bs->bh->b_data))
		kfree(s->base);

	return error;

cleanup_dquot:
	dquot_free_block(inode, EXT4_C2B(EXT4_SB(sb), 1));
	goto cleanup;

bad_block:
	EXT4_ERROR_INODE(inode, "bad block %llu",
			 EXT4_I(inode)->i_file_acl);
	goto cleanup;

#undef header
}