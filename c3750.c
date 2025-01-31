static void *bpf_any_get(void *raw, enum bpf_type type)
{
	switch (type) {
	case BPF_TYPE_PROG:
		atomic_inc(&((struct bpf_prog *)raw)->aux->refcnt);
		break;
	case BPF_TYPE_MAP:
		bpf_map_inc(raw, true);
		break;
	default:
		WARN_ON_ONCE(1);
		break;
	}

	return raw;
}