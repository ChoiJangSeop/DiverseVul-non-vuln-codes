netscreen_seek_read(wtap *wth, gint64 seek_off,
	struct wtap_pkthdr *phdr, Buffer *buf,
	int *err, gchar **err_info)
{
	int		pkt_len;
	char		line[NETSCREEN_LINE_LENGTH];
	char		cap_int[NETSCREEN_MAX_INT_NAME_LENGTH];
	gboolean	cap_dir;
	char		cap_dst[13];

	if (file_seek(wth->random_fh, seek_off, SEEK_SET, err) == -1) {
		return FALSE;
	}

	if (file_gets(line, NETSCREEN_LINE_LENGTH, wth->random_fh) == NULL) {
		*err = file_error(wth->random_fh, err_info);
		if (*err == 0) {
			*err = WTAP_ERR_SHORT_READ;
		}
		return FALSE;
	}

	pkt_len = parse_netscreen_rec_hdr(phdr, line, cap_int, &cap_dir,
	    cap_dst, err, err_info);
	if (pkt_len == -1)
		return FALSE;

	if (!parse_netscreen_hex_dump(wth->random_fh, pkt_len, cap_int,
	    cap_dst, phdr, buf, err, err_info))
		return FALSE;
	return TRUE;
}