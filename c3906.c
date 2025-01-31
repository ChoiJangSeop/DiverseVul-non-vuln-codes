parse_netscreen_packet(FILE_T fh, struct wtap_pkthdr *phdr, Buffer* buf,
    char *line, int *err, gchar **err_info)
{
	int		sec;
	int		dsec;
	char		cap_int[NETSCREEN_MAX_INT_NAME_LENGTH];
	char		direction[2];
	guint		pkt_len;
	char		cap_src[13];
	char		cap_dst[13];
	guint8		*pd;
	gchar		*p;
	int		n, i = 0;
	guint		offset = 0;
	gchar		dststr[13];

	phdr->rec_type = REC_TYPE_PACKET;
	phdr->presence_flags = WTAP_HAS_TS|WTAP_HAS_CAP_LEN;

	if (sscanf(line, "%9d.%9d: %15[a-z0-9/:.-](%1[io]) len=%9u:%12s->%12s/",
		   &sec, &dsec, cap_int, direction, &pkt_len, cap_src, cap_dst) < 5) {
		*err = WTAP_ERR_BAD_FILE;
		*err_info = g_strdup("netscreen: Can't parse packet-header");
		return -1;
	}
	if (pkt_len > WTAP_MAX_PACKET_SIZE) {
		/*
		 * Probably a corrupt capture file; don't blow up trying
		 * to allocate space for an immensely-large packet.
		 */
		*err = WTAP_ERR_BAD_FILE;
		*err_info = g_strdup_printf("netscreen: File has %u-byte packet, bigger than maximum of %u",
		    pkt_len, WTAP_MAX_PACKET_SIZE);
		return FALSE;
	}

	/*
	 * If direction[0] is 'o', the direction is NETSCREEN_EGRESS,
	 * otherwise it's NETSCREEN_INGRESS.
	 */

	phdr->ts.secs  = sec;
	phdr->ts.nsecs = dsec * 100000000;
	phdr->len = pkt_len;

	/* Make sure we have enough room for the packet */
	ws_buffer_assure_space(buf, pkt_len);
	pd = ws_buffer_start_ptr(buf);

	while(1) {

		/* The last packet is not delimited by an empty line, but by EOF
		 * So accept EOF as a valid delimiter too
		 */
		if (file_gets(line, NETSCREEN_LINE_LENGTH, fh) == NULL) {
			break;
		}

		/*
		 * Skip blanks.
		 * The number of blanks is not fixed - for wireless
		 * interfaces, there may be 14 extra spaces before
		 * the hex data.
		 */
		for (p = &line[0]; g_ascii_isspace(*p); p++)
			;
		/* packets are delimited with empty lines */
		if (*p == '\0') {
			break;
		}

		n = parse_single_hex_dump_line(p, pd, offset);

		/* the smallest packet has a length of 6 bytes, if
		 * the first hex-data is less then check whether
		 * it is a info-line and act accordingly
		 */
		if (offset == 0 && n < 6) {
			if (info_line(line)) {
				if (++i <= NETSCREEN_MAX_INFOLINES) {
					continue;
				}
			} else {
				*err = WTAP_ERR_BAD_FILE;
				*err_info = g_strdup("netscreen: cannot parse hex-data");
				return FALSE;
			}
		}

		/* If there is no more data and the line was not empty,
		 * then there must be an error in the file
		 */
		if (n == -1) {
			*err = WTAP_ERR_BAD_FILE;
			*err_info = g_strdup("netscreen: cannot parse hex-data");
			return FALSE;
		}

		/* Adjust the offset to the data that was just added to the buffer */
		offset += n;

		/* If there was more hex-data than was announced in the len=x
		 * header, then then there must be an error in the file
		 */
		if (offset > pkt_len) {
			*err = WTAP_ERR_BAD_FILE;
			*err_info = g_strdup("netscreen: too much hex-data");
			return FALSE;
		}
	}

	/*
	 * Determine the encapsulation type, based on the
	 * first 4 characters of the interface name
	 *
	 * XXX	convert this to a 'case' structure when adding more
	 *	(non-ethernet) interfacetypes
	 */
	if (strncmp(cap_int, "adsl", 4) == 0) {
		/* The ADSL interface can be bridged with or without
		 * PPP encapsulation. Check whether the first six bytes
		 * of the hex data are the same as the destination mac
		 * address in the header. If they are, assume ethernet
		 * LinkLayer or else PPP
		 */
		g_snprintf(dststr, 13, "%02x%02x%02x%02x%02x%02x",
		   pd[0], pd[1], pd[2], pd[3], pd[4], pd[5]);
		if (strncmp(dststr, cap_dst, 12) == 0)
			phdr->pkt_encap = WTAP_ENCAP_ETHERNET;
		else
			phdr->pkt_encap = WTAP_ENCAP_PPP;
		}
	else if (strncmp(cap_int, "seri", 4) == 0)
		phdr->pkt_encap = WTAP_ENCAP_PPP;
	else
		phdr->pkt_encap = WTAP_ENCAP_ETHERNET;

	phdr->caplen = offset;

	return TRUE;
}