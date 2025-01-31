static int list_locations(struct kmem_cache *s, char *buf,
					enum track_item alloc)
{
	int len = 0;
	unsigned long i;
	struct loc_track t = { 0, 0, NULL };
	int node;

	if (!alloc_loc_track(&t, PAGE_SIZE / sizeof(struct location),
			GFP_TEMPORARY))
		return sprintf(buf, "Out of memory\n");

	/* Push back cpu slabs */
	flush_all(s);

	for_each_node_state(node, N_NORMAL_MEMORY) {
		struct kmem_cache_node *n = get_node(s, node);
		unsigned long flags;
		struct page *page;

		if (!atomic_long_read(&n->nr_slabs))
			continue;

		spin_lock_irqsave(&n->list_lock, flags);
		list_for_each_entry(page, &n->partial, lru)
			process_slab(&t, s, page, alloc);
		list_for_each_entry(page, &n->full, lru)
			process_slab(&t, s, page, alloc);
		spin_unlock_irqrestore(&n->list_lock, flags);
	}

	for (i = 0; i < t.count; i++) {
		struct location *l = &t.loc[i];

		if (len > PAGE_SIZE - 100)
			break;
		len += sprintf(buf + len, "%7ld ", l->count);

		if (l->addr)
			len += sprint_symbol(buf + len, (unsigned long)l->addr);
		else
			len += sprintf(buf + len, "<not-available>");

		if (l->sum_time != l->min_time) {
			unsigned long remainder;

			len += sprintf(buf + len, " age=%ld/%ld/%ld",
			l->min_time,
			div_long_long_rem(l->sum_time, l->count, &remainder),
			l->max_time);
		} else
			len += sprintf(buf + len, " age=%ld",
				l->min_time);

		if (l->min_pid != l->max_pid)
			len += sprintf(buf + len, " pid=%ld-%ld",
				l->min_pid, l->max_pid);
		else
			len += sprintf(buf + len, " pid=%ld",
				l->min_pid);

		if (num_online_cpus() > 1 && !cpus_empty(l->cpus) &&
				len < PAGE_SIZE - 60) {
			len += sprintf(buf + len, " cpus=");
			len += cpulist_scnprintf(buf + len, PAGE_SIZE - len - 50,
					l->cpus);
		}

		if (num_online_nodes() > 1 && !nodes_empty(l->nodes) &&
				len < PAGE_SIZE - 60) {
			len += sprintf(buf + len, " nodes=");
			len += nodelist_scnprintf(buf + len, PAGE_SIZE - len - 50,
					l->nodes);
		}

		len += sprintf(buf + len, "\n");
	}

	free_loc_track(&t);
	if (!t.count)
		len += sprintf(buf, "No data\n");
	return len;
}