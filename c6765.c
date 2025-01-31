static ssize_t map_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos,
			 int cap_setid,
			 struct uid_gid_map *map,
			 struct uid_gid_map *parent_map)
{
	struct seq_file *seq = file->private_data;
	struct user_namespace *ns = seq->private;
	struct uid_gid_map new_map;
	unsigned idx;
	struct uid_gid_extent extent;
	char *kbuf = NULL, *pos, *next_line;
	ssize_t ret;

	/* Only allow < page size writes at the beginning of the file */
	if ((*ppos != 0) || (count >= PAGE_SIZE))
		return -EINVAL;

	/* Slurp in the user data */
	kbuf = memdup_user_nul(buf, count);
	if (IS_ERR(kbuf))
		return PTR_ERR(kbuf);

	/*
	 * The userns_state_mutex serializes all writes to any given map.
	 *
	 * Any map is only ever written once.
	 *
	 * An id map fits within 1 cache line on most architectures.
	 *
	 * On read nothing needs to be done unless you are on an
	 * architecture with a crazy cache coherency model like alpha.
	 *
	 * There is a one time data dependency between reading the
	 * count of the extents and the values of the extents.  The
	 * desired behavior is to see the values of the extents that
	 * were written before the count of the extents.
	 *
	 * To achieve this smp_wmb() is used on guarantee the write
	 * order and smp_rmb() is guaranteed that we don't have crazy
	 * architectures returning stale data.
	 */
	mutex_lock(&userns_state_mutex);

	memset(&new_map, 0, sizeof(struct uid_gid_map));

	ret = -EPERM;
	/* Only allow one successful write to the map */
	if (map->nr_extents != 0)
		goto out;

	/*
	 * Adjusting namespace settings requires capabilities on the target.
	 */
	if (cap_valid(cap_setid) && !file_ns_capable(file, ns, CAP_SYS_ADMIN))
		goto out;

	/* Parse the user data */
	ret = -EINVAL;
	pos = kbuf;
	for (; pos; pos = next_line) {

		/* Find the end of line and ensure I don't look past it */
		next_line = strchr(pos, '\n');
		if (next_line) {
			*next_line = '\0';
			next_line++;
			if (*next_line == '\0')
				next_line = NULL;
		}

		pos = skip_spaces(pos);
		extent.first = simple_strtoul(pos, &pos, 10);
		if (!isspace(*pos))
			goto out;

		pos = skip_spaces(pos);
		extent.lower_first = simple_strtoul(pos, &pos, 10);
		if (!isspace(*pos))
			goto out;

		pos = skip_spaces(pos);
		extent.count = simple_strtoul(pos, &pos, 10);
		if (*pos && !isspace(*pos))
			goto out;

		/* Verify there is not trailing junk on the line */
		pos = skip_spaces(pos);
		if (*pos != '\0')
			goto out;

		/* Verify we have been given valid starting values */
		if ((extent.first == (u32) -1) ||
		    (extent.lower_first == (u32) -1))
			goto out;

		/* Verify count is not zero and does not cause the
		 * extent to wrap
		 */
		if ((extent.first + extent.count) <= extent.first)
			goto out;
		if ((extent.lower_first + extent.count) <=
		     extent.lower_first)
			goto out;

		/* Do the ranges in extent overlap any previous extents? */
		if (mappings_overlap(&new_map, &extent))
			goto out;

		if ((new_map.nr_extents + 1) == UID_GID_MAP_MAX_EXTENTS &&
		    (next_line != NULL))
			goto out;

		ret = insert_extent(&new_map, &extent);
		if (ret < 0)
			goto out;
		ret = -EINVAL;
	}
	/* Be very certaint the new map actually exists */
	if (new_map.nr_extents == 0)
		goto out;

	ret = -EPERM;
	/* Validate the user is allowed to use user id's mapped to. */
	if (!new_idmap_permitted(file, ns, cap_setid, &new_map))
		goto out;

	ret = sort_idmaps(&new_map);
	if (ret < 0)
		goto out;

	ret = -EPERM;
	/* Map the lower ids from the parent user namespace to the
	 * kernel global id space.
	 */
	for (idx = 0; idx < new_map.nr_extents; idx++) {
		struct uid_gid_extent *e;
		u32 lower_first;

		if (new_map.nr_extents <= UID_GID_MAP_MAX_BASE_EXTENTS)
			e = &new_map.extent[idx];
		else
			e = &new_map.forward[idx];

		lower_first = map_id_range_down(parent_map,
						e->lower_first,
						e->count);

		/* Fail if we can not map the specified extent to
		 * the kernel global id space.
		 */
		if (lower_first == (u32) -1)
			goto out;

		e->lower_first = lower_first;
	}

	/* Install the map */
	if (new_map.nr_extents <= UID_GID_MAP_MAX_BASE_EXTENTS) {
		memcpy(map->extent, new_map.extent,
		       new_map.nr_extents * sizeof(new_map.extent[0]));
	} else {
		map->forward = new_map.forward;
		map->reverse = new_map.reverse;
	}
	smp_wmb();
	map->nr_extents = new_map.nr_extents;

	*ppos = count;
	ret = count;
out:
	if (ret < 0 && new_map.nr_extents > UID_GID_MAP_MAX_BASE_EXTENTS) {
		kfree(new_map.forward);
		kfree(new_map.reverse);
		map->forward = NULL;
		map->reverse = NULL;
		map->nr_extents = 0;
	}

	mutex_unlock(&userns_state_mutex);
	kfree(kbuf);
	return ret;
}