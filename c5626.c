long keyctl_chown_key(key_serial_t id, uid_t user, gid_t group)
{
	struct key_user *newowner, *zapowner = NULL;
	struct key *key;
	key_ref_t key_ref;
	long ret;
	kuid_t uid;
	kgid_t gid;

	uid = make_kuid(current_user_ns(), user);
	gid = make_kgid(current_user_ns(), group);
	ret = -EINVAL;
	if ((user != (uid_t) -1) && !uid_valid(uid))
		goto error;
	if ((group != (gid_t) -1) && !gid_valid(gid))
		goto error;

	ret = 0;
	if (user == (uid_t) -1 && group == (gid_t) -1)
		goto error;

	key_ref = lookup_user_key(id, KEY_LOOKUP_CREATE | KEY_LOOKUP_PARTIAL,
				  KEY_NEED_SETATTR);
	if (IS_ERR(key_ref)) {
		ret = PTR_ERR(key_ref);
		goto error;
	}

	key = key_ref_to_ptr(key_ref);

	/* make the changes with the locks held to prevent chown/chown races */
	ret = -EACCES;
	down_write(&key->sem);

	if (!capable(CAP_SYS_ADMIN)) {
		/* only the sysadmin can chown a key to some other UID */
		if (user != (uid_t) -1 && !uid_eq(key->uid, uid))
			goto error_put;

		/* only the sysadmin can set the key's GID to a group other
		 * than one of those that the current process subscribes to */
		if (group != (gid_t) -1 && !gid_eq(gid, key->gid) && !in_group_p(gid))
			goto error_put;
	}

	/* change the UID */
	if (user != (uid_t) -1 && !uid_eq(uid, key->uid)) {
		ret = -ENOMEM;
		newowner = key_user_lookup(uid);
		if (!newowner)
			goto error_put;

		/* transfer the quota burden to the new user */
		if (test_bit(KEY_FLAG_IN_QUOTA, &key->flags)) {
			unsigned maxkeys = uid_eq(uid, GLOBAL_ROOT_UID) ?
				key_quota_root_maxkeys : key_quota_maxkeys;
			unsigned maxbytes = uid_eq(uid, GLOBAL_ROOT_UID) ?
				key_quota_root_maxbytes : key_quota_maxbytes;

			spin_lock(&newowner->lock);
			if (newowner->qnkeys + 1 >= maxkeys ||
			    newowner->qnbytes + key->quotalen >= maxbytes ||
			    newowner->qnbytes + key->quotalen <
			    newowner->qnbytes)
				goto quota_overrun;

			newowner->qnkeys++;
			newowner->qnbytes += key->quotalen;
			spin_unlock(&newowner->lock);

			spin_lock(&key->user->lock);
			key->user->qnkeys--;
			key->user->qnbytes -= key->quotalen;
			spin_unlock(&key->user->lock);
		}

		atomic_dec(&key->user->nkeys);
		atomic_inc(&newowner->nkeys);

		if (test_bit(KEY_FLAG_INSTANTIATED, &key->flags)) {
			atomic_dec(&key->user->nikeys);
			atomic_inc(&newowner->nikeys);
		}

		zapowner = key->user;
		key->user = newowner;
		key->uid = uid;
	}

	/* change the GID */
	if (group != (gid_t) -1)
		key->gid = gid;

	ret = 0;

error_put:
	up_write(&key->sem);
	key_put(key);
	if (zapowner)
		key_user_put(zapowner);
error:
	return ret;

quota_overrun:
	spin_unlock(&newowner->lock);
	zapowner = newowner;
	ret = -EDQUOT;
	goto error_put;
}