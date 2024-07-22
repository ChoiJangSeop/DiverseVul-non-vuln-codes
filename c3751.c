void bpf_map_inc(struct bpf_map *map, bool uref)
{
	atomic_inc(&map->refcnt);
	if (uref)
		atomic_inc(&map->usercnt);
}