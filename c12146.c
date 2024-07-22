ext4_mb_mark_diskspace_used(struct ext4_allocation_context *ac,
				handle_t *handle, unsigned int reserv_clstrs)
{
	struct buffer_head *bitmap_bh = NULL;
	struct ext4_group_desc *gdp;
	struct buffer_head *gdp_bh;
	struct ext4_sb_info *sbi;
	struct super_block *sb;
	ext4_fsblk_t block;
	int err, len;

	BUG_ON(ac->ac_status != AC_STATUS_FOUND);
	BUG_ON(ac->ac_b_ex.fe_len <= 0);

	sb = ac->ac_sb;
	sbi = EXT4_SB(sb);

	bitmap_bh = ext4_read_block_bitmap(sb, ac->ac_b_ex.fe_group);
	if (IS_ERR(bitmap_bh)) {
		err = PTR_ERR(bitmap_bh);
		bitmap_bh = NULL;
		goto out_err;
	}

	BUFFER_TRACE(bitmap_bh, "getting write access");
	err = ext4_journal_get_write_access(handle, bitmap_bh);
	if (err)
		goto out_err;

	err = -EIO;
	gdp = ext4_get_group_desc(sb, ac->ac_b_ex.fe_group, &gdp_bh);
	if (!gdp)
		goto out_err;

	ext4_debug("using block group %u(%d)\n", ac->ac_b_ex.fe_group,
			ext4_free_group_clusters(sb, gdp));

	BUFFER_TRACE(gdp_bh, "get_write_access");
	err = ext4_journal_get_write_access(handle, gdp_bh);
	if (err)
		goto out_err;

	block = ext4_grp_offs_to_block(sb, &ac->ac_b_ex);

	len = EXT4_C2B(sbi, ac->ac_b_ex.fe_len);
	if (!ext4_data_block_valid(sbi, block, len)) {
		ext4_error(sb, "Allocating blocks %llu-%llu which overlap "
			   "fs metadata", block, block+len);
		/* File system mounted not to panic on error
		 * Fix the bitmap and return EFSCORRUPTED
		 * We leak some of the blocks here.
		 */
		ext4_lock_group(sb, ac->ac_b_ex.fe_group);
		ext4_set_bits(bitmap_bh->b_data, ac->ac_b_ex.fe_start,
			      ac->ac_b_ex.fe_len);
		ext4_unlock_group(sb, ac->ac_b_ex.fe_group);
		err = ext4_handle_dirty_metadata(handle, NULL, bitmap_bh);
		if (!err)
			err = -EFSCORRUPTED;
		goto out_err;
	}

	ext4_lock_group(sb, ac->ac_b_ex.fe_group);
#ifdef AGGRESSIVE_CHECK
	{
		int i;
		for (i = 0; i < ac->ac_b_ex.fe_len; i++) {
			BUG_ON(mb_test_bit(ac->ac_b_ex.fe_start + i,
						bitmap_bh->b_data));
		}
	}
#endif
	ext4_set_bits(bitmap_bh->b_data, ac->ac_b_ex.fe_start,
		      ac->ac_b_ex.fe_len);
	if (ext4_has_group_desc_csum(sb) &&
	    (gdp->bg_flags & cpu_to_le16(EXT4_BG_BLOCK_UNINIT))) {
		gdp->bg_flags &= cpu_to_le16(~EXT4_BG_BLOCK_UNINIT);
		ext4_free_group_clusters_set(sb, gdp,
					     ext4_free_clusters_after_init(sb,
						ac->ac_b_ex.fe_group, gdp));
	}
	len = ext4_free_group_clusters(sb, gdp) - ac->ac_b_ex.fe_len;
	ext4_free_group_clusters_set(sb, gdp, len);
	ext4_block_bitmap_csum_set(sb, ac->ac_b_ex.fe_group, gdp, bitmap_bh);
	ext4_group_desc_csum_set(sb, ac->ac_b_ex.fe_group, gdp);

	ext4_unlock_group(sb, ac->ac_b_ex.fe_group);
	percpu_counter_sub(&sbi->s_freeclusters_counter, ac->ac_b_ex.fe_len);
	/*
	 * Now reduce the dirty block count also. Should not go negative
	 */
	if (!(ac->ac_flags & EXT4_MB_DELALLOC_RESERVED))
		/* release all the reserved blocks if non delalloc */
		percpu_counter_sub(&sbi->s_dirtyclusters_counter,
				   reserv_clstrs);

	if (sbi->s_log_groups_per_flex) {
		ext4_group_t flex_group = ext4_flex_group(sbi,
							  ac->ac_b_ex.fe_group);
		atomic64_sub(ac->ac_b_ex.fe_len,
			     &sbi_array_rcu_deref(sbi, s_flex_groups,
						  flex_group)->free_clusters);
	}

	err = ext4_handle_dirty_metadata(handle, NULL, bitmap_bh);
	if (err)
		goto out_err;
	err = ext4_handle_dirty_metadata(handle, NULL, gdp_bh);

out_err:
	brelse(bitmap_bh);
	return err;
}