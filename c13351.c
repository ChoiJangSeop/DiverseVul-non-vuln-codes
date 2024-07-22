hugetlbfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode * inode;
	struct dentry * root;
	int ret;
	struct hugetlbfs_config config;
	struct hugetlbfs_sb_info *sbinfo;

	save_mount_options(sb, data);

	config.nr_blocks = -1; /* No limit on size by default */
	config.nr_inodes = -1; /* No limit on number of inodes by default */
	config.uid = current_fsuid();
	config.gid = current_fsgid();
	config.mode = 0755;
	config.hstate = &default_hstate;
	ret = hugetlbfs_parse_options(data, &config);
	if (ret)
		return ret;

	sbinfo = kmalloc(sizeof(struct hugetlbfs_sb_info), GFP_KERNEL);
	if (!sbinfo)
		return -ENOMEM;
	sb->s_fs_info = sbinfo;
	sbinfo->hstate = config.hstate;
	spin_lock_init(&sbinfo->stat_lock);
	sbinfo->max_blocks = config.nr_blocks;
	sbinfo->free_blocks = config.nr_blocks;
	sbinfo->max_inodes = config.nr_inodes;
	sbinfo->free_inodes = config.nr_inodes;
	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_blocksize = huge_page_size(config.hstate);
	sb->s_blocksize_bits = huge_page_shift(config.hstate);
	sb->s_magic = HUGETLBFS_MAGIC;
	sb->s_op = &hugetlbfs_ops;
	sb->s_time_gran = 1;
	inode = hugetlbfs_get_root(sb, &config);
	if (!inode)
		goto out_free;

	root = d_alloc_root(inode);
	if (!root) {
		iput(inode);
		goto out_free;
	}
	sb->s_root = root;
	return 0;
out_free:
	kfree(sbinfo);
	return -ENOMEM;
}