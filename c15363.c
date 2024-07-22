static void umount_tree(struct mount *mnt, enum umount_tree_flags how)
{
	LIST_HEAD(tmp_list);
	struct mount *p;

	if (how & UMOUNT_PROPAGATE)
		propagate_mount_unlock(mnt);

	/* Gather the mounts to umount */
	for (p = mnt; p; p = next_mnt(p, mnt)) {
		p->mnt.mnt_flags |= MNT_UMOUNT;
		list_move(&p->mnt_list, &tmp_list);
	}

	/* Hide the mounts from mnt_mounts */
	list_for_each_entry(p, &tmp_list, mnt_list) {
		list_del_init(&p->mnt_child);
	}

	/* Add propogated mounts to the tmp_list */
	if (how & UMOUNT_PROPAGATE)
		propagate_umount(&tmp_list);

	while (!list_empty(&tmp_list)) {
		p = list_first_entry(&tmp_list, struct mount, mnt_list);
		list_del_init(&p->mnt_expire);
		list_del_init(&p->mnt_list);
		__touch_mnt_namespace(p->mnt_ns);
		p->mnt_ns = NULL;
		if (how & UMOUNT_SYNC)
			p->mnt.mnt_flags |= MNT_SYNC_UMOUNT;

		pin_insert_group(&p->mnt_umount, &p->mnt_parent->mnt, &unmounted);
		if (mnt_has_parent(p)) {
			mnt_add_count(p->mnt_parent, -1);
			umount_mnt(p);
		}
		change_mnt_propagation(p, MS_PRIVATE);
	}
}