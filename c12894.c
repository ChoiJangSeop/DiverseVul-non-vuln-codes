static void inotify_free_group_priv(struct fsnotify_group *group)
{
	/* ideally the idr is empty and we won't hit the BUG in teh callback */
	idr_for_each(&group->inotify_data.idr, idr_callback, group);
	idr_remove_all(&group->inotify_data.idr);
	idr_destroy(&group->inotify_data.idr);
	free_uid(group->inotify_data.user);
}