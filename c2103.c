static int process_one_ticket(struct ceph_auth_client *ac,
			      struct ceph_crypto_key *secret,
			      void **p, void *end,
			      void *dbuf, void *ticket_buf)
{
	struct ceph_x_info *xi = ac->private;
	int type;
	u8 tkt_struct_v, blob_struct_v;
	struct ceph_x_ticket_handler *th;
	void *dp, *dend;
	int dlen;
	char is_enc;
	struct timespec validity;
	struct ceph_crypto_key old_key;
	void *tp, *tpend;
	struct ceph_timespec new_validity;
	struct ceph_crypto_key new_session_key;
	struct ceph_buffer *new_ticket_blob;
	unsigned long new_expires, new_renew_after;
	u64 new_secret_id;
	int ret;

	ceph_decode_need(p, end, sizeof(u32) + 1, bad);

	type = ceph_decode_32(p);
	dout(" ticket type %d %s\n", type, ceph_entity_type_name(type));

	tkt_struct_v = ceph_decode_8(p);
	if (tkt_struct_v != 1)
		goto bad;

	th = get_ticket_handler(ac, type);
	if (IS_ERR(th)) {
		ret = PTR_ERR(th);
		goto out;
	}

	/* blob for me */
	dlen = ceph_x_decrypt(secret, p, end, dbuf,
			      TEMP_TICKET_BUF_LEN);
	if (dlen <= 0) {
		ret = dlen;
		goto out;
	}
	dout(" decrypted %d bytes\n", dlen);
	dp = dbuf;
	dend = dp + dlen;

	tkt_struct_v = ceph_decode_8(&dp);
	if (tkt_struct_v != 1)
		goto bad;

	memcpy(&old_key, &th->session_key, sizeof(old_key));
	ret = ceph_crypto_key_decode(&new_session_key, &dp, dend);
	if (ret)
		goto out;

	ceph_decode_copy(&dp, &new_validity, sizeof(new_validity));
	ceph_decode_timespec(&validity, &new_validity);
	new_expires = get_seconds() + validity.tv_sec;
	new_renew_after = new_expires - (validity.tv_sec / 4);
	dout(" expires=%lu renew_after=%lu\n", new_expires,
	     new_renew_after);

	/* ticket blob for service */
	ceph_decode_8_safe(p, end, is_enc, bad);
	tp = ticket_buf;
	if (is_enc) {
		/* encrypted */
		dout(" encrypted ticket\n");
		dlen = ceph_x_decrypt(&old_key, p, end, ticket_buf,
				      TEMP_TICKET_BUF_LEN);
		if (dlen < 0) {
			ret = dlen;
			goto out;
		}
		dlen = ceph_decode_32(&tp);
	} else {
		/* unencrypted */
		ceph_decode_32_safe(p, end, dlen, bad);
		ceph_decode_need(p, end, dlen, bad);
		ceph_decode_copy(p, ticket_buf, dlen);
	}
	tpend = tp + dlen;
	dout(" ticket blob is %d bytes\n", dlen);
	ceph_decode_need(&tp, tpend, 1 + sizeof(u64), bad);
	blob_struct_v = ceph_decode_8(&tp);
	new_secret_id = ceph_decode_64(&tp);
	ret = ceph_decode_buffer(&new_ticket_blob, &tp, tpend);
	if (ret)
		goto out;

	/* all is well, update our ticket */
	ceph_crypto_key_destroy(&th->session_key);
	if (th->ticket_blob)
		ceph_buffer_put(th->ticket_blob);
	th->session_key = new_session_key;
	th->ticket_blob = new_ticket_blob;
	th->validity = new_validity;
	th->secret_id = new_secret_id;
	th->expires = new_expires;
	th->renew_after = new_renew_after;
	dout(" got ticket service %d (%s) secret_id %lld len %d\n",
	     type, ceph_entity_type_name(type), th->secret_id,
	     (int)th->ticket_blob->vec.iov_len);
	xi->have_keys |= th->service;

out:
	return ret;

bad:
	ret = -EINVAL;
	goto out;
}