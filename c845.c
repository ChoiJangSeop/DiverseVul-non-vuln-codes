int hfsplus_rename_cat(u32 cnid,
		       struct inode *src_dir, struct qstr *src_name,
		       struct inode *dst_dir, struct qstr *dst_name)
{
	struct super_block *sb = src_dir->i_sb;
	struct hfs_find_data src_fd, dst_fd;
	hfsplus_cat_entry entry;
	int entry_size, type;
	int err;

	dprint(DBG_CAT_MOD, "rename_cat: %u - %lu,%s - %lu,%s\n",
		cnid, src_dir->i_ino, src_name->name,
		dst_dir->i_ino, dst_name->name);
	err = hfs_find_init(HFSPLUS_SB(sb)->cat_tree, &src_fd);
	if (err)
		return err;
	dst_fd = src_fd;

	/* find the old dir entry and read the data */
	hfsplus_cat_build_key(sb, src_fd.search_key, src_dir->i_ino, src_name);
	err = hfs_brec_find(&src_fd);
	if (err)
		goto out;

	hfs_bnode_read(src_fd.bnode, &entry, src_fd.entryoffset,
				src_fd.entrylength);

	/* create new dir entry with the data from the old entry */
	hfsplus_cat_build_key(sb, dst_fd.search_key, dst_dir->i_ino, dst_name);
	err = hfs_brec_find(&dst_fd);
	if (err != -ENOENT) {
		if (!err)
			err = -EEXIST;
		goto out;
	}

	err = hfs_brec_insert(&dst_fd, &entry, src_fd.entrylength);
	if (err)
		goto out;
	dst_dir->i_size++;
	dst_dir->i_mtime = dst_dir->i_ctime = CURRENT_TIME_SEC;

	/* finally remove the old entry */
	hfsplus_cat_build_key(sb, src_fd.search_key, src_dir->i_ino, src_name);
	err = hfs_brec_find(&src_fd);
	if (err)
		goto out;
	err = hfs_brec_remove(&src_fd);
	if (err)
		goto out;
	src_dir->i_size--;
	src_dir->i_mtime = src_dir->i_ctime = CURRENT_TIME_SEC;

	/* remove old thread entry */
	hfsplus_cat_build_key(sb, src_fd.search_key, cnid, NULL);
	err = hfs_brec_find(&src_fd);
	if (err)
		goto out;
	type = hfs_bnode_read_u16(src_fd.bnode, src_fd.entryoffset);
	err = hfs_brec_remove(&src_fd);
	if (err)
		goto out;

	/* create new thread entry */
	hfsplus_cat_build_key(sb, dst_fd.search_key, cnid, NULL);
	entry_size = hfsplus_fill_cat_thread(sb, &entry, type,
		dst_dir->i_ino, dst_name);
	err = hfs_brec_find(&dst_fd);
	if (err != -ENOENT) {
		if (!err)
			err = -EEXIST;
		goto out;
	}
	err = hfs_brec_insert(&dst_fd, &entry, entry_size);

	hfsplus_mark_inode_dirty(dst_dir, HFSPLUS_I_CAT_DIRTY);
	hfsplus_mark_inode_dirty(src_dir, HFSPLUS_I_CAT_DIRTY);
out:
	hfs_bnode_put(dst_fd.bnode);
	hfs_find_exit(&src_fd);
	return err;
}