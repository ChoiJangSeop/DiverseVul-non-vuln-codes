static ssize_t proc_pid_cmdline_read(struct file *file, char __user *buf,
				     size_t _count, loff_t *pos)
{
	struct task_struct *tsk;
	struct mm_struct *mm;
	char *page;
	unsigned long count = _count;
	unsigned long arg_start, arg_end, env_start, env_end;
	unsigned long len1, len2, len;
	unsigned long p;
	char c;
	ssize_t rv;

	BUG_ON(*pos < 0);

	tsk = get_proc_task(file_inode(file));
	if (!tsk)
		return -ESRCH;
	mm = get_task_mm(tsk);
	put_task_struct(tsk);
	if (!mm)
		return 0;
	/* Check if process spawned far enough to have cmdline. */
	if (!mm->env_end) {
		rv = 0;
		goto out_mmput;
	}

	page = (char *)__get_free_page(GFP_KERNEL);
	if (!page) {
		rv = -ENOMEM;
		goto out_mmput;
	}

	down_read(&mm->mmap_sem);
	arg_start = mm->arg_start;
	arg_end = mm->arg_end;
	env_start = mm->env_start;
	env_end = mm->env_end;
	up_read(&mm->mmap_sem);

	BUG_ON(arg_start > arg_end);
	BUG_ON(env_start > env_end);

	len1 = arg_end - arg_start;
	len2 = env_end - env_start;

	/* Empty ARGV. */
	if (len1 == 0) {
		rv = 0;
		goto out_free_page;
	}
	/*
	 * Inherently racy -- command line shares address space
	 * with code and data.
	 */
	rv = access_remote_vm(mm, arg_end - 1, &c, 1, 0);
	if (rv <= 0)
		goto out_free_page;

	rv = 0;

	if (c == '\0') {
		/* Command line (set of strings) occupies whole ARGV. */
		if (len1 <= *pos)
			goto out_free_page;

		p = arg_start + *pos;
		len = len1 - *pos;
		while (count > 0 && len > 0) {
			unsigned int _count;
			int nr_read;

			_count = min3(count, len, PAGE_SIZE);
			nr_read = access_remote_vm(mm, p, page, _count, 0);
			if (nr_read < 0)
				rv = nr_read;
			if (nr_read <= 0)
				goto out_free_page;

			if (copy_to_user(buf, page, nr_read)) {
				rv = -EFAULT;
				goto out_free_page;
			}

			p	+= nr_read;
			len	-= nr_read;
			buf	+= nr_read;
			count	-= nr_read;
			rv	+= nr_read;
		}
	} else {
		/*
		 * Command line (1 string) occupies ARGV and
		 * extends into ENVP.
		 */
		struct {
			unsigned long p;
			unsigned long len;
		} cmdline[2] = {
			{ .p = arg_start, .len = len1 },
			{ .p = env_start, .len = len2 },
		};
		loff_t pos1 = *pos;
		unsigned int i;

		i = 0;
		while (i < 2 && pos1 >= cmdline[i].len) {
			pos1 -= cmdline[i].len;
			i++;
		}
		while (i < 2) {
			p = cmdline[i].p + pos1;
			len = cmdline[i].len - pos1;
			while (count > 0 && len > 0) {
				unsigned int _count, l;
				int nr_read;
				bool final;

				_count = min3(count, len, PAGE_SIZE);
				nr_read = access_remote_vm(mm, p, page, _count, 0);
				if (nr_read < 0)
					rv = nr_read;
				if (nr_read <= 0)
					goto out_free_page;

				/*
				 * Command line can be shorter than whole ARGV
				 * even if last "marker" byte says it is not.
				 */
				final = false;
				l = strnlen(page, nr_read);
				if (l < nr_read) {
					nr_read = l;
					final = true;
				}

				if (copy_to_user(buf, page, nr_read)) {
					rv = -EFAULT;
					goto out_free_page;
				}

				p	+= nr_read;
				len	-= nr_read;
				buf	+= nr_read;
				count	-= nr_read;
				rv	+= nr_read;

				if (final)
					goto out_free_page;
			}

			/* Only first chunk can be read partially. */
			pos1 = 0;
			i++;
		}
	}

out_free_page:
	free_page((unsigned long)page);
out_mmput:
	mmput(mm);
	if (rv > 0)
		*pos += rv;
	return rv;
}