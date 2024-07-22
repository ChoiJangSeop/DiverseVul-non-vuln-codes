static int proc_keys_show(struct seq_file *m, void *v)
{
	struct rb_node *_p = v;
	struct key *key = rb_entry(_p, struct key, serial_node);
	struct timespec now;
	unsigned long timo;
	key_ref_t key_ref, skey_ref;
	char xbuf[16];
	int rc;

	struct keyring_search_context ctx = {
		.index_key.type		= key->type,
		.index_key.description	= key->description,
		.cred			= m->file->f_cred,
		.match_data.cmp		= lookup_user_key_possessed,
		.match_data.raw_data	= key,
		.match_data.lookup_type	= KEYRING_SEARCH_LOOKUP_DIRECT,
		.flags			= KEYRING_SEARCH_NO_STATE_CHECK,
	};

	key_ref = make_key_ref(key, 0);

	/* determine if the key is possessed by this process (a test we can
	 * skip if the key does not indicate the possessor can view it
	 */
	if (key->perm & KEY_POS_VIEW) {
		skey_ref = search_my_process_keyrings(&ctx);
		if (!IS_ERR(skey_ref)) {
			key_ref_put(skey_ref);
			key_ref = make_key_ref(key, 1);
		}
	}

	/* check whether the current task is allowed to view the key */
	rc = key_task_permission(key_ref, ctx.cred, KEY_NEED_VIEW);
	if (rc < 0)
		return 0;

	now = current_kernel_time();

	rcu_read_lock();

	/* come up with a suitable timeout value */
	if (key->expiry == 0) {
		memcpy(xbuf, "perm", 5);
	} else if (now.tv_sec >= key->expiry) {
		memcpy(xbuf, "expd", 5);
	} else {
		timo = key->expiry - now.tv_sec;

		if (timo < 60)
			sprintf(xbuf, "%lus", timo);
		else if (timo < 60*60)
			sprintf(xbuf, "%lum", timo / 60);
		else if (timo < 60*60*24)
			sprintf(xbuf, "%luh", timo / (60*60));
		else if (timo < 60*60*24*7)
			sprintf(xbuf, "%lud", timo / (60*60*24));
		else
			sprintf(xbuf, "%luw", timo / (60*60*24*7));
	}

#define showflag(KEY, LETTER, FLAG) \
	(test_bit(FLAG,	&(KEY)->flags) ? LETTER : '-')

	seq_printf(m, "%08x %c%c%c%c%c%c%c %5d %4s %08x %5d %5d %-9.9s ",
		   key->serial,
		   showflag(key, 'I', KEY_FLAG_INSTANTIATED),
		   showflag(key, 'R', KEY_FLAG_REVOKED),
		   showflag(key, 'D', KEY_FLAG_DEAD),
		   showflag(key, 'Q', KEY_FLAG_IN_QUOTA),
		   showflag(key, 'U', KEY_FLAG_USER_CONSTRUCT),
		   showflag(key, 'N', KEY_FLAG_NEGATIVE),
		   showflag(key, 'i', KEY_FLAG_INVALIDATED),
		   refcount_read(&key->usage),
		   xbuf,
		   key->perm,
		   from_kuid_munged(seq_user_ns(m), key->uid),
		   from_kgid_munged(seq_user_ns(m), key->gid),
		   key->type->name);

#undef showflag

	if (key->type->describe)
		key->type->describe(key, m);
	seq_putc(m, '\n');

	rcu_read_unlock();
	return 0;
}