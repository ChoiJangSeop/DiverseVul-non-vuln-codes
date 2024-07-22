int ext4_page_mkwrite(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page = vmf->page;
	loff_t size;
	unsigned long len;
	int ret;
	struct file *file = vma->vm_file;
	struct inode *inode = file_inode(file);
	struct address_space *mapping = inode->i_mapping;
	handle_t *handle;
	get_block_t *get_block;
	int retries = 0;

	sb_start_pagefault(inode->i_sb);
	file_update_time(vma->vm_file);
	/* Delalloc case is easy... */
	if (test_opt(inode->i_sb, DELALLOC) &&
	    !ext4_should_journal_data(inode) &&
	    !ext4_nonda_switch(inode->i_sb)) {
		do {
			ret = block_page_mkwrite(vma, vmf,
						   ext4_da_get_block_prep);
		} while (ret == -ENOSPC &&
		       ext4_should_retry_alloc(inode->i_sb, &retries));
		goto out_ret;
	}

	lock_page(page);
	size = i_size_read(inode);
	/* Page got truncated from under us? */
	if (page->mapping != mapping || page_offset(page) > size) {
		unlock_page(page);
		ret = VM_FAULT_NOPAGE;
		goto out;
	}

	if (page->index == size >> PAGE_CACHE_SHIFT)
		len = size & ~PAGE_CACHE_MASK;
	else
		len = PAGE_CACHE_SIZE;
	/*
	 * Return if we have all the buffers mapped. This avoids the need to do
	 * journal_start/journal_stop which can block and take a long time
	 */
	if (page_has_buffers(page)) {
		if (!ext4_walk_page_buffers(NULL, page_buffers(page),
					    0, len, NULL,
					    ext4_bh_unmapped)) {
			/* Wait so that we don't change page under IO */
			wait_for_stable_page(page);
			ret = VM_FAULT_LOCKED;
			goto out;
		}
	}
	unlock_page(page);
	/* OK, we need to fill the hole... */
	if (ext4_should_dioread_nolock(inode))
		get_block = ext4_get_block_write;
	else
		get_block = ext4_get_block;
retry_alloc:
	handle = ext4_journal_start(inode, EXT4_HT_WRITE_PAGE,
				    ext4_writepage_trans_blocks(inode));
	if (IS_ERR(handle)) {
		ret = VM_FAULT_SIGBUS;
		goto out;
	}
	ret = block_page_mkwrite(vma, vmf, get_block);
	if (!ret && ext4_should_journal_data(inode)) {
		if (ext4_walk_page_buffers(handle, page_buffers(page), 0,
			  PAGE_CACHE_SIZE, NULL, do_journal_get_write_access)) {
			unlock_page(page);
			ret = VM_FAULT_SIGBUS;
			ext4_journal_stop(handle);
			goto out;
		}
		ext4_set_inode_state(inode, EXT4_STATE_JDATA);
	}
	ext4_journal_stop(handle);
	if (ret == -ENOSPC && ext4_should_retry_alloc(inode->i_sb, &retries))
		goto retry_alloc;
out_ret:
	ret = block_page_mkwrite_return(ret);
out:
	sb_end_pagefault(inode->i_sb);
	return ret;
}