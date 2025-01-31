static int ext4_dax_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	int result;
	handle_t *handle = NULL;
	struct super_block *sb = file_inode(vma->vm_file)->i_sb;
	bool write = vmf->flags & FAULT_FLAG_WRITE;

	if (write) {
		sb_start_pagefault(sb);
		file_update_time(vma->vm_file);
		handle = ext4_journal_start_sb(sb, EXT4_HT_WRITE_PAGE,
						EXT4_DATA_TRANS_BLOCKS(sb));
	}

	if (IS_ERR(handle))
		result = VM_FAULT_SIGBUS;
	else
		result = __dax_fault(vma, vmf, ext4_get_block_dax,
						ext4_end_io_unwritten);

	if (write) {
		if (!IS_ERR(handle))
			ext4_journal_stop(handle);
		sb_end_pagefault(sb);
	}

	return result;
}