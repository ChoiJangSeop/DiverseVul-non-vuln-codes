static int asn1_find_indefinite_length(const unsigned char *data, size_t datalen,
				       size_t *_dp, size_t *_len,
				       const char **_errmsg)
{
	unsigned char tag, tmp;
	size_t dp = *_dp, len, n;
	int indef_level = 1;

next_tag:
	if (unlikely(datalen - dp < 2)) {
		if (datalen == dp)
			goto missing_eoc;
		goto data_overrun_error;
	}

	/* Extract a tag from the data */
	tag = data[dp++];
	if (tag == 0) {
		/* It appears to be an EOC. */
		if (data[dp++] != 0)
			goto invalid_eoc;
		if (--indef_level <= 0) {
			*_len = dp - *_dp;
			*_dp = dp;
			return 0;
		}
		goto next_tag;
	}

	if (unlikely((tag & 0x1f) == ASN1_LONG_TAG)) {
		do {
			if (unlikely(datalen - dp < 2))
				goto data_overrun_error;
			tmp = data[dp++];
		} while (tmp & 0x80);
	}

	/* Extract the length */
	len = data[dp++];
	if (len <= 0x7f) {
		dp += len;
		goto next_tag;
	}

	if (unlikely(len == ASN1_INDEFINITE_LENGTH)) {
		/* Indefinite length */
		if (unlikely((tag & ASN1_CONS_BIT) == ASN1_PRIM << 5))
			goto indefinite_len_primitive;
		indef_level++;
		goto next_tag;
	}

	n = len - 0x80;
	if (unlikely(n > sizeof(size_t) - 1))
		goto length_too_long;
	if (unlikely(n > datalen - dp))
		goto data_overrun_error;
	for (len = 0; n > 0; n--) {
		len <<= 8;
		len |= data[dp++];
	}
	dp += len;
	goto next_tag;

length_too_long:
	*_errmsg = "Unsupported length";
	goto error;
indefinite_len_primitive:
	*_errmsg = "Indefinite len primitive not permitted";
	goto error;
invalid_eoc:
	*_errmsg = "Invalid length EOC";
	goto error;
data_overrun_error:
	*_errmsg = "Data overrun error";
	goto error;
missing_eoc:
	*_errmsg = "Missing EOC in indefinite len cons";
error:
	*_dp = dp;
	return -1;
}