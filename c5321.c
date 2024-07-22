static int nbd_negotiate_send_rep_list(QIOChannel *ioc, NBDExport *exp)
{
    size_t name_len, desc_len;
    uint32_t len;
    const char *name = exp->name ? exp->name : "";
    const char *desc = exp->description ? exp->description : "";
    int rc;

    TRACE("Advertising export name '%s' description '%s'", name, desc);
    name_len = strlen(name);
    desc_len = strlen(desc);
    len = name_len + desc_len + sizeof(len);
    rc = nbd_negotiate_send_rep_len(ioc, NBD_REP_SERVER, NBD_OPT_LIST, len);
    if (rc < 0) {
        return rc;
    }

    len = cpu_to_be32(name_len);
    if (nbd_negotiate_write(ioc, &len, sizeof(len)) < 0) {
        LOG("write failed (name length)");
        return -EINVAL;
    }
    if (nbd_negotiate_write(ioc, name, name_len) < 0) {
        LOG("write failed (name buffer)");
        return -EINVAL;
    }
    if (nbd_negotiate_write(ioc, desc, desc_len) < 0) {
        LOG("write failed (description buffer)");
        return -EINVAL;
    }
    return 0;
}