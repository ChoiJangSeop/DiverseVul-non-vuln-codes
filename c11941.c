static int local_unlinkat(FsContext *ctx, V9fsPath *dir,
                          const char *name, int flags)
{
    int ret;
    int dirfd;

    dirfd = local_opendir_nofollow(ctx, dir->data);
    if (dirfd == -1) {
        return -1;
    }

    ret = local_unlinkat_common(ctx, dirfd, name, flags);
    close_preserve_errno(dirfd);
    return ret;
}