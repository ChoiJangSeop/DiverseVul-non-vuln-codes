static int load_script(struct linux_binprm *bprm)
{
	const char *i_arg, *i_name;
	char *cp;
	struct file *file;
	char interp[BINPRM_BUF_SIZE];
	int retval;

	if ((bprm->buf[0] != '#') || (bprm->buf[1] != '!') ||
	    (bprm->recursion_depth > BINPRM_MAX_RECURSION))
		return -ENOEXEC;
	/*
	 * This section does the #! interpretation.
	 * Sorta complicated, but hopefully it will work.  -TYT
	 */

	bprm->recursion_depth++;
	allow_write_access(bprm->file);
	fput(bprm->file);
	bprm->file = NULL;

	bprm->buf[BINPRM_BUF_SIZE - 1] = '\0';
	if ((cp = strchr(bprm->buf, '\n')) == NULL)
		cp = bprm->buf+BINPRM_BUF_SIZE-1;
	*cp = '\0';
	while (cp > bprm->buf) {
		cp--;
		if ((*cp == ' ') || (*cp == '\t'))
			*cp = '\0';
		else
			break;
	}
	for (cp = bprm->buf+2; (*cp == ' ') || (*cp == '\t'); cp++);
	if (*cp == '\0') 
		return -ENOEXEC; /* No interpreter name found */
	i_name = cp;
	i_arg = NULL;
	for ( ; *cp && (*cp != ' ') && (*cp != '\t'); cp++)
		/* nothing */ ;
	while ((*cp == ' ') || (*cp == '\t'))
		*cp++ = '\0';
	if (*cp)
		i_arg = cp;
	strcpy_s(interp, sizeof(interp), i_name);
	/*
	 * OK, we've parsed out the interpreter name and
	 * (optional) argument.
	 * Splice in (1) the interpreter's name for argv[0]
	 *           (2) (optional) argument to interpreter
	 *           (3) filename of shell script (replace argv[0])
	 *
	 * This is done in reverse order, because of how the
	 * user environment and arguments are stored.
	 */
	retval = remove_arg_zero(bprm);
	if (retval)
		return retval;
	retval = copy_strings_kernel(1, &bprm->interp, bprm);
	if (retval < 0) return retval; 
	bprm->argc++;
	if (i_arg) {
		retval = copy_strings_kernel(1, &i_arg, bprm);
		if (retval < 0) return retval; 
		bprm->argc++;
	}
	retval = copy_strings_kernel(1, &i_name, bprm);
	if (retval) return retval; 
	bprm->argc++;
	bprm->interp = interp;

	/*
	 * OK, now restart the process with the interpreter's dentry.
	 */
	file = open_exec(interp);
	if (IS_ERR(file))
		return PTR_ERR(file);

	bprm->file = file;
	retval = prepare_binprm(bprm);
	if (retval < 0)
		return retval;
	return search_binary_handler(bprm);
}