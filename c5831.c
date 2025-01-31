static void lo_release(struct gendisk *disk, fmode_t mode)
{
	struct loop_device *lo = disk->private_data;
	int err;

	if (atomic_dec_return(&lo->lo_refcnt))
		return;

	mutex_lock(&lo->lo_ctl_mutex);
	if (lo->lo_flags & LO_FLAGS_AUTOCLEAR) {
		/*
		 * In autoclear mode, stop the loop thread
		 * and remove configuration after last close.
		 */
		err = loop_clr_fd(lo);
		if (!err)
			return;
	} else if (lo->lo_state == Lo_bound) {
		/*
		 * Otherwise keep thread (if running) and config,
		 * but flush possible ongoing bios in thread.
		 */
		blk_mq_freeze_queue(lo->lo_queue);
		blk_mq_unfreeze_queue(lo->lo_queue);
	}

	mutex_unlock(&lo->lo_ctl_mutex);
}