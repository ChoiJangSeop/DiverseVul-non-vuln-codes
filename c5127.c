static int rxrpc_krb5_decode_ticket(u8 **_ticket, u16 *_tktlen,
				    const __be32 **_xdr, unsigned int *_toklen)
{
	const __be32 *xdr = *_xdr;
	unsigned int toklen = *_toklen, len;

	/* there must be at least one length word */
	if (toklen <= 4)
		return -EINVAL;

	_enter(",{%x},%u", ntohl(xdr[0]), toklen);

	len = ntohl(*xdr++);
	toklen -= 4;
	if (len > AFSTOKEN_K5_TIX_MAX)
		return -EINVAL;
	*_tktlen = len;

	_debug("ticket len %u", len);

	if (len > 0) {
		*_ticket = kmemdup(xdr, len, GFP_KERNEL);
		if (!*_ticket)
			return -ENOMEM;
		len = (len + 3) & ~3;
		toklen -= len;
		xdr += len >> 2;
	}

	*_xdr = xdr;
	*_toklen = toklen;
	_leave(" = 0 [toklen=%u]", toklen);
	return 0;
}