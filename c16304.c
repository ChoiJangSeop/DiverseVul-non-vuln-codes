void ssl_set_client_disabled(SSL *s)
	{
	CERT *c = s->cert;
	c->mask_a = 0;
	c->mask_k = 0;
	/* Don't allow TLS 1.2 only ciphers if we don't suppport them */
	if (!SSL_CLIENT_USE_TLS1_2_CIPHERS(s))
		c->mask_ssl = SSL_TLSV1_2;
	else
		c->mask_ssl = 0;
	ssl_set_sig_mask(&c->mask_a, s, SSL_SECOP_SIGALG_MASK);
	/* Disable static DH if we don't include any appropriate
	 * signature algorithms.
	 */
	if (c->mask_a & SSL_aRSA)
		c->mask_k |= SSL_kDHr|SSL_kECDHr;
	if (c->mask_a & SSL_aDSS)
		c->mask_k |= SSL_kDHd;
	if (c->mask_a & SSL_aECDSA)
		c->mask_k |= SSL_kECDHe;
#ifndef OPENSSL_NO_KRB5
	if (!kssl_tgt_is_available(s->kssl_ctx))
		{
		c->mask_a |= SSL_aKRB5;
		c->mask_k |= SSL_kKRB5;
		}
#endif
#ifndef OPENSSL_NO_PSK
	/* with PSK there must be client callback set */
	if (!s->psk_client_callback)
		{
		c->mask_a |= SSL_aPSK;
		c->mask_k |= SSL_kPSK;
		}
#endif /* OPENSSL_NO_PSK */
	c->valid = 1;
	}