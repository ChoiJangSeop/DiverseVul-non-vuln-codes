asmlinkage long compat_sys_keyctl(u32 option,
				  u32 arg2, u32 arg3, u32 arg4, u32 arg5)
{
	switch (option) {
	case KEYCTL_GET_KEYRING_ID:
		return keyctl_get_keyring_ID(arg2, arg3);

	case KEYCTL_JOIN_SESSION_KEYRING:
		return keyctl_join_session_keyring(compat_ptr(arg2));

	case KEYCTL_UPDATE:
		return keyctl_update_key(arg2, compat_ptr(arg3), arg4);

	case KEYCTL_REVOKE:
		return keyctl_revoke_key(arg2);

	case KEYCTL_DESCRIBE:
		return keyctl_describe_key(arg2, compat_ptr(arg3), arg4);

	case KEYCTL_CLEAR:
		return keyctl_keyring_clear(arg2);

	case KEYCTL_LINK:
		return keyctl_keyring_link(arg2, arg3);

	case KEYCTL_UNLINK:
		return keyctl_keyring_unlink(arg2, arg3);

	case KEYCTL_SEARCH:
		return keyctl_keyring_search(arg2, compat_ptr(arg3),
					     compat_ptr(arg4), arg5);

	case KEYCTL_READ:
		return keyctl_read_key(arg2, compat_ptr(arg3), arg4);

	case KEYCTL_CHOWN:
		return keyctl_chown_key(arg2, arg3, arg4);

	case KEYCTL_SETPERM:
		return keyctl_setperm_key(arg2, arg3);

	case KEYCTL_INSTANTIATE:
		return keyctl_instantiate_key(arg2, compat_ptr(arg3), arg4,
					      arg5);

	case KEYCTL_NEGATE:
		return keyctl_negate_key(arg2, arg3, arg4);

	case KEYCTL_SET_REQKEY_KEYRING:
		return keyctl_set_reqkey_keyring(arg2);

	case KEYCTL_SET_TIMEOUT:
		return keyctl_set_timeout(arg2, arg3);

	case KEYCTL_ASSUME_AUTHORITY:
		return keyctl_assume_authority(arg2);

	case KEYCTL_GET_SECURITY:
		return keyctl_get_security(arg2, compat_ptr(arg3), arg4);

	default:
		return -EOPNOTSUPP;
	}

} /* end compat_sys_keyctl() */