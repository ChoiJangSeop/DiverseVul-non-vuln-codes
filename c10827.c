_archive_write_disk_header(struct archive *_a, struct archive_entry *entry)
{
	struct archive_write_disk *a = (struct archive_write_disk *)_a;
	struct fixup_entry *fe;
	const char *linkname;
	int ret, r;

	archive_check_magic(&a->archive, ARCHIVE_WRITE_DISK_MAGIC,
	    ARCHIVE_STATE_HEADER | ARCHIVE_STATE_DATA,
	    "archive_write_disk_header");
	archive_clear_error(&a->archive);
	if (a->archive.state & ARCHIVE_STATE_DATA) {
		r = _archive_write_disk_finish_entry(&a->archive);
		if (r == ARCHIVE_FATAL)
			return (r);
	}

	/* Set up for this particular entry. */
	a->pst = NULL;
	a->current_fixup = NULL;
	a->deferred = 0;
	if (a->entry) {
		archive_entry_free(a->entry);
		a->entry = NULL;
	}
	a->entry = archive_entry_clone(entry);
	a->fd = -1;
	a->fd_offset = 0;
	a->offset = 0;
	a->restore_pwd = -1;
	a->uid = a->user_uid;
	a->mode = archive_entry_mode(a->entry);
	if (archive_entry_size_is_set(a->entry))
		a->filesize = archive_entry_size(a->entry);
	else
		a->filesize = -1;
	archive_strcpy(&(a->_name_data), archive_entry_pathname(a->entry));
	a->name = a->_name_data.s;
	archive_clear_error(&a->archive);

	/*
	 * Clean up the requested path.  This is necessary for correct
	 * dir restores; the dir restore logic otherwise gets messed
	 * up by nonsense like "dir/.".
	 */
	ret = cleanup_pathname(a);
	if (ret != ARCHIVE_OK)
		return (ret);

	/*
	 * Check if we have a hardlink that points to itself.
	 */
	linkname = archive_entry_hardlink(a->entry);
	if (linkname != NULL && strcmp(a->name, linkname) == 0) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
		    "Skipping hardlink pointing to itself: %s",
		    a->name);
		return (ARCHIVE_WARN);
	}

	/*
	 * Query the umask so we get predictable mode settings.
	 * This gets done on every call to _write_header in case the
	 * user edits their umask during the extraction for some
	 * reason.
	 */
	umask(a->user_umask = umask(0));

	/* Figure out what we need to do for this entry. */
	a->todo = TODO_MODE_BASE;
	if (a->flags & ARCHIVE_EXTRACT_PERM) {
		a->todo |= TODO_MODE_FORCE; /* Be pushy about permissions. */
		/*
		 * SGID requires an extra "check" step because we
		 * cannot easily predict the GID that the system will
		 * assign.  (Different systems assign GIDs to files
		 * based on a variety of criteria, including process
		 * credentials and the gid of the enclosing
		 * directory.)  We can only restore the SGID bit if
		 * the file has the right GID, and we only know the
		 * GID if we either set it (see set_ownership) or if
		 * we've actually called stat() on the file after it
		 * was restored.  Since there are several places at
		 * which we might verify the GID, we need a TODO bit
		 * to keep track.
		 */
		if (a->mode & S_ISGID)
			a->todo |= TODO_SGID | TODO_SGID_CHECK;
		/*
		 * Verifying the SUID is simpler, but can still be
		 * done in multiple ways, hence the separate "check" bit.
		 */
		if (a->mode & S_ISUID)
			a->todo |= TODO_SUID | TODO_SUID_CHECK;
	} else {
		/*
		 * User didn't request full permissions, so don't
		 * restore SUID, SGID bits and obey umask.
		 */
		a->mode &= ~S_ISUID;
		a->mode &= ~S_ISGID;
		a->mode &= ~S_ISVTX;
		a->mode &= ~a->user_umask;
	}
	if (a->flags & ARCHIVE_EXTRACT_OWNER)
		a->todo |= TODO_OWNER;
	if (a->flags & ARCHIVE_EXTRACT_TIME)
		a->todo |= TODO_TIMES;
	if (a->flags & ARCHIVE_EXTRACT_ACL) {
#if ARCHIVE_ACL_DARWIN
		/*
		 * On MacOS, platform ACLs get stored in mac_metadata, too.
		 * If we intend to extract mac_metadata and it is present
		 * we skip extracting libarchive NFSv4 ACLs.
		 */
		size_t metadata_size;

		if ((a->flags & ARCHIVE_EXTRACT_MAC_METADATA) == 0 ||
		    archive_entry_mac_metadata(a->entry,
		    &metadata_size) == NULL || metadata_size == 0)
#endif
#if ARCHIVE_ACL_LIBRICHACL
		/*
		 * RichACLs are stored in an extended attribute.
		 * If we intend to extract extended attributes and have this
		 * attribute we skip extracting libarchive NFSv4 ACLs.
		 */
		short extract_acls = 1;
		if (a->flags & ARCHIVE_EXTRACT_XATTR && (
		    archive_entry_acl_types(a->entry) &
		    ARCHIVE_ENTRY_ACL_TYPE_NFS4)) {
			const char *attr_name;
			const void *attr_value;
			size_t attr_size;
			int i = archive_entry_xattr_reset(a->entry);
			while (i--) {
				archive_entry_xattr_next(a->entry, &attr_name,
				    &attr_value, &attr_size);
				if (attr_name != NULL && attr_value != NULL &&
				    attr_size > 0 && strcmp(attr_name,
				    "trusted.richacl") == 0) {
					extract_acls = 0;
					break;
				}
			}
		}
		if (extract_acls)
#endif
#if ARCHIVE_ACL_DARWIN || ARCHIVE_ACL_LIBRICHACL
		{
#endif
		if (archive_entry_filetype(a->entry) == AE_IFDIR)
			a->deferred |= TODO_ACLS;
		else
			a->todo |= TODO_ACLS;
#if ARCHIVE_ACL_DARWIN || ARCHIVE_ACL_LIBRICHACL
		}
#endif
	}
	if (a->flags & ARCHIVE_EXTRACT_MAC_METADATA) {
		if (archive_entry_filetype(a->entry) == AE_IFDIR)
			a->deferred |= TODO_MAC_METADATA;
		else
			a->todo |= TODO_MAC_METADATA;
	}
#if defined(__APPLE__) && defined(UF_COMPRESSED) && defined(HAVE_ZLIB_H)
	if ((a->flags & ARCHIVE_EXTRACT_NO_HFS_COMPRESSION) == 0) {
		unsigned long set, clear;
		archive_entry_fflags(a->entry, &set, &clear);
		if ((set & ~clear) & UF_COMPRESSED) {
			a->todo |= TODO_HFS_COMPRESSION;
			a->decmpfs_block_count = (unsigned)-1;
		}
	}
	if ((a->flags & ARCHIVE_EXTRACT_HFS_COMPRESSION_FORCED) != 0 &&
	    (a->mode & AE_IFMT) == AE_IFREG && a->filesize > 0) {
		a->todo |= TODO_HFS_COMPRESSION;
		a->decmpfs_block_count = (unsigned)-1;
	}
	{
		const char *p;

		/* Check if the current file name is a type of the
		 * resource fork file. */
		p = strrchr(a->name, '/');
		if (p == NULL)
			p = a->name;
		else
			p++;
		if (p[0] == '.' && p[1] == '_') {
			/* Do not compress "._XXX" files. */
			a->todo &= ~TODO_HFS_COMPRESSION;
			if (a->filesize > 0)
				a->todo |= TODO_APPLEDOUBLE;
		}
	}
#endif

	if (a->flags & ARCHIVE_EXTRACT_XATTR) {
#if ARCHIVE_XATTR_DARWIN
		/*
		 * On MacOS, extended attributes get stored in mac_metadata,
		 * too. If we intend to extract mac_metadata and it is present
		 * we skip extracting extended attributes.
		 */
		size_t metadata_size;

		if ((a->flags & ARCHIVE_EXTRACT_MAC_METADATA) == 0 ||
		    archive_entry_mac_metadata(a->entry,
		    &metadata_size) == NULL || metadata_size == 0)
#endif
		a->todo |= TODO_XATTR;
	}
	if (a->flags & ARCHIVE_EXTRACT_FFLAGS)
		a->todo |= TODO_FFLAGS;
	if (a->flags & ARCHIVE_EXTRACT_SECURE_SYMLINKS) {
		ret = check_symlinks(a);
		if (ret != ARCHIVE_OK)
			return (ret);
	}
#if defined(HAVE_FCHDIR) && defined(PATH_MAX)
	/* If path exceeds PATH_MAX, shorten the path. */
	edit_deep_directories(a);
#endif

	ret = restore_entry(a);

#if defined(__APPLE__) && defined(UF_COMPRESSED) && defined(HAVE_ZLIB_H)
	/*
	 * Check if the filesystem the file is restoring on supports
	 * HFS+ Compression. If not, cancel HFS+ Compression.
	 */
	if (a->todo | TODO_HFS_COMPRESSION) {
		/*
		 * NOTE: UF_COMPRESSED is ignored even if the filesystem
		 * supports HFS+ Compression because the file should
		 * have at least an extended attribute "com.apple.decmpfs"
		 * before the flag is set to indicate that the file have
		 * been compressed. If the filesystem does not support
		 * HFS+ Compression the system call will fail.
		 */
		if (a->fd < 0 || fchflags(a->fd, UF_COMPRESSED) != 0)
			a->todo &= ~TODO_HFS_COMPRESSION;
	}
#endif

	/*
	 * TODO: There are rumours that some extended attributes must
	 * be restored before file data is written.  If this is true,
	 * then we either need to write all extended attributes both
	 * before and after restoring the data, or find some rule for
	 * determining which must go first and which last.  Due to the
	 * many ways people are using xattrs, this may prove to be an
	 * intractable problem.
	 */

#ifdef HAVE_FCHDIR
	/* If we changed directory above, restore it here. */
	if (a->restore_pwd >= 0) {
		r = fchdir(a->restore_pwd);
		if (r != 0) {
			archive_set_error(&a->archive, errno,
			    "chdir() failure");
			ret = ARCHIVE_FATAL;
		}
		close(a->restore_pwd);
		a->restore_pwd = -1;
	}
#endif

	/*
	 * Fixup uses the unedited pathname from archive_entry_pathname(),
	 * because it is relative to the base dir and the edited path
	 * might be relative to some intermediate dir as a result of the
	 * deep restore logic.
	 */
	if (a->deferred & TODO_MODE) {
		fe = current_fixup(a, archive_entry_pathname(entry));
		if (fe == NULL)
			return (ARCHIVE_FATAL);
		fe->fixup |= TODO_MODE_BASE;
		fe->mode = a->mode;
	}

	if ((a->deferred & TODO_TIMES)
		&& (archive_entry_mtime_is_set(entry)
		    || archive_entry_atime_is_set(entry))) {
		fe = current_fixup(a, archive_entry_pathname(entry));
		if (fe == NULL)
			return (ARCHIVE_FATAL);
		fe->mode = a->mode;
		fe->fixup |= TODO_TIMES;
		if (archive_entry_atime_is_set(entry)) {
			fe->atime = archive_entry_atime(entry);
			fe->atime_nanos = archive_entry_atime_nsec(entry);
		} else {
			/* If atime is unset, use start time. */
			fe->atime = a->start_time;
			fe->atime_nanos = 0;
		}
		if (archive_entry_mtime_is_set(entry)) {
			fe->mtime = archive_entry_mtime(entry);
			fe->mtime_nanos = archive_entry_mtime_nsec(entry);
		} else {
			/* If mtime is unset, use start time. */
			fe->mtime = a->start_time;
			fe->mtime_nanos = 0;
		}
		if (archive_entry_birthtime_is_set(entry)) {
			fe->birthtime = archive_entry_birthtime(entry);
			fe->birthtime_nanos = archive_entry_birthtime_nsec(
			    entry);
		} else {
			/* If birthtime is unset, use mtime. */
			fe->birthtime = fe->mtime;
			fe->birthtime_nanos = fe->mtime_nanos;
		}
	}

	if (a->deferred & TODO_ACLS) {
		fe = current_fixup(a, archive_entry_pathname(entry));
		if (fe == NULL)
			return (ARCHIVE_FATAL);
		fe->fixup |= TODO_ACLS;
		archive_acl_copy(&fe->acl, archive_entry_acl(entry));
	}

	if (a->deferred & TODO_MAC_METADATA) {
		const void *metadata;
		size_t metadata_size;
		metadata = archive_entry_mac_metadata(a->entry, &metadata_size);
		if (metadata != NULL && metadata_size > 0) {
			fe = current_fixup(a, archive_entry_pathname(entry));
			if (fe == NULL)
				return (ARCHIVE_FATAL);
			fe->mac_metadata = malloc(metadata_size);
			if (fe->mac_metadata != NULL) {
				memcpy(fe->mac_metadata, metadata,
				    metadata_size);
				fe->mac_metadata_size = metadata_size;
				fe->fixup |= TODO_MAC_METADATA;
			}
		}
	}

	if (a->deferred & TODO_FFLAGS) {
		fe = current_fixup(a, archive_entry_pathname(entry));
		if (fe == NULL)
			return (ARCHIVE_FATAL);
		fe->fixup |= TODO_FFLAGS;
		/* TODO: Complete this.. defer fflags from below. */
	}

	/* We've created the object and are ready to pour data into it. */
	if (ret >= ARCHIVE_WARN)
		a->archive.state = ARCHIVE_STATE_DATA;
	/*
	 * If it's not open, tell our client not to try writing.
	 * In particular, dirs, links, etc, don't get written to.
	 */
	if (a->fd < 0) {
		archive_entry_set_size(entry, 0);
		a->filesize = 0;
	}

	return (ret);
}