static int apparmor_setprocattr(struct task_struct *task, char *name,
				void *value, size_t size)
{
	struct common_audit_data sa;
	struct apparmor_audit_data aad = {0,};
	char *command, *args = value;
	size_t arg_size;
	int error;

	if (size == 0)
		return -EINVAL;
	/* args points to a PAGE_SIZE buffer, AppArmor requires that
	 * the buffer must be null terminated or have size <= PAGE_SIZE -1
	 * so that AppArmor can null terminate them
	 */
	if (args[size - 1] != '\0') {
		if (size == PAGE_SIZE)
			return -EINVAL;
		args[size] = '\0';
	}

	/* task can only write its own attributes */
	if (current != task)
		return -EACCES;

	args = value;
	args = strim(args);
	command = strsep(&args, " ");
	if (!args)
		return -EINVAL;
	args = skip_spaces(args);
	if (!*args)
		return -EINVAL;

	arg_size = size - (args - (char *) value);
	if (strcmp(name, "current") == 0) {
		if (strcmp(command, "changehat") == 0) {
			error = aa_setprocattr_changehat(args, arg_size,
							 !AA_DO_TEST);
		} else if (strcmp(command, "permhat") == 0) {
			error = aa_setprocattr_changehat(args, arg_size,
							 AA_DO_TEST);
		} else if (strcmp(command, "changeprofile") == 0) {
			error = aa_setprocattr_changeprofile(args, !AA_ONEXEC,
							     !AA_DO_TEST);
		} else if (strcmp(command, "permprofile") == 0) {
			error = aa_setprocattr_changeprofile(args, !AA_ONEXEC,
							     AA_DO_TEST);
		} else
			goto fail;
	} else if (strcmp(name, "exec") == 0) {
		if (strcmp(command, "exec") == 0)
			error = aa_setprocattr_changeprofile(args, AA_ONEXEC,
							     !AA_DO_TEST);
		else
			goto fail;
	} else
		/* only support the "current" and "exec" process attributes */
		return -EINVAL;

	if (!error)
		error = size;
	return error;

fail:
	sa.type = LSM_AUDIT_DATA_NONE;
	sa.aad = &aad;
	aad.profile = aa_current_profile();
	aad.op = OP_SETPROCATTR;
	aad.info = name;
	aad.error = -EINVAL;
	aa_audit_msg(AUDIT_APPARMOR_DENIED, &sa, NULL);
	return -EINVAL;
}