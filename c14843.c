void __init proc_root_init(void)
{
	struct vfsmount *mnt;
	int err;

	proc_init_inodecache();
	err = register_filesystem(&proc_fs_type);
	if (err)
		return;
	mnt = kern_mount_data(&proc_fs_type, &init_pid_ns);
	if (IS_ERR(mnt)) {
		unregister_filesystem(&proc_fs_type);
		return;
	}

	init_pid_ns.proc_mnt = mnt;
	proc_symlink("mounts", NULL, "self/mounts");

	proc_net_init();

#ifdef CONFIG_SYSVIPC
	proc_mkdir("sysvipc", NULL);
#endif
	proc_mkdir("fs", NULL);
	proc_mkdir("driver", NULL);
	proc_mkdir("fs/nfsd", NULL); /* somewhere for the nfsd filesystem to be mounted */
#if defined(CONFIG_SUN_OPENPROMFS) || defined(CONFIG_SUN_OPENPROMFS_MODULE)
	/* just give it a mountpoint */
	proc_mkdir("openprom", NULL);
#endif
	proc_tty_init();
#ifdef CONFIG_PROC_DEVICETREE
	proc_device_tree_init();
#endif
	proc_mkdir("bus", NULL);
	proc_sys_init();
}