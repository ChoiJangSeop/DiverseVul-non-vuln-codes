int ssl3_get_client_hello(SSL *s)
	{
	int i,j,ok,al,ret= -1;
	unsigned int cookie_len;
	long n;
	unsigned long id;
	unsigned char *p,*d,*q;
	SSL_CIPHER *c;
#ifndef OPENSSL_NO_COMP
	SSL_COMP *comp=NULL;
#endif
	STACK_OF(SSL_CIPHER) *ciphers=NULL;

	/* We do this so that we will respond with our native type.
	 * If we are TLSv1 and we get SSLv3, we will respond with TLSv1,
	 * This down switching should be handled by a different method.
	 * If we are SSLv3, we will respond with SSLv3, even if prompted with
	 * TLSv1.
	 */
	if (s->state == SSL3_ST_SR_CLNT_HELLO_A)
		{
		s->first_packet=1;
		s->state=SSL3_ST_SR_CLNT_HELLO_B;
		}
	n=s->method->ssl_get_message(s,
		SSL3_ST_SR_CLNT_HELLO_B,
		SSL3_ST_SR_CLNT_HELLO_C,
		SSL3_MT_CLIENT_HELLO,
		SSL3_RT_MAX_PLAIN_LENGTH,
		&ok);

	if (!ok) return((int)n);
	d=p=(unsigned char *)s->init_msg;

	/* use version from inside client hello, not from record header
	 * (may differ: see RFC 2246, Appendix E, second paragraph) */
	s->client_version=(((int)p[0])<<8)|(int)p[1];
	p+=2;

	if (s->client_version < s->version)
		{
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO, SSL_R_WRONG_VERSION_NUMBER);
		if ((s->client_version>>8) == SSL3_VERSION_MAJOR) 
			{
			/* similar to ssl3_get_record, send alert using remote version number */
			s->version = s->client_version;
			}
		al = SSL_AD_PROTOCOL_VERSION;
		goto f_err;
		}

	/* load the client random */
	memcpy(s->s3->client_random,p,SSL3_RANDOM_SIZE);
	p+=SSL3_RANDOM_SIZE;

	/* get the session-id */
	j= *(p++);

	s->hit=0;
	/* Versions before 0.9.7 always allow session reuse during renegotiation
	 * (i.e. when s->new_session is true), option
	 * SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION is new with 0.9.7.
	 * Maybe this optional behaviour should always have been the default,
	 * but we cannot safely change the default behaviour (or new applications
	 * might be written that become totally unsecure when compiled with
	 * an earlier library version)
	 */
	if (j == 0 || (s->new_session && (s->options & SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION)))
		{
		if (!ssl_get_new_session(s,1))
			goto err;
		}
	else
		{
		i=ssl_get_prev_session(s,p,j);
		if (i == 1)
			{ /* previous session */
			s->hit=1;
			}
		else if (i == -1)
			goto err;
		else /* i == 0 */
			{
			if (!ssl_get_new_session(s,1))
				goto err;
			}
		}

	p+=j;

	if (SSL_version(s) == DTLS1_VERSION)
		{
		/* cookie stuff */
		cookie_len = *(p++);

		if ( (SSL_get_options(s) & SSL_OP_COOKIE_EXCHANGE) &&
			s->d1->send_cookie == 0)
			{
			/* HelloVerifyMessage has already been sent */
			if ( cookie_len != s->d1->cookie_len)
				{
				al = SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO, SSL_R_COOKIE_MISMATCH);
				goto f_err;
				}
			}

		/* 
		 * The ClientHello may contain a cookie even if the
		 * HelloVerify message has not been sent--make sure that it
		 * does not cause an overflow.
		 */
		if ( cookie_len > sizeof(s->d1->rcvd_cookie))
			{
			/* too much data */
			al = SSL_AD_DECODE_ERROR;
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO, SSL_R_COOKIE_MISMATCH);
			goto f_err;
			}

		/* verify the cookie if appropriate option is set. */
		if ( (SSL_get_options(s) & SSL_OP_COOKIE_EXCHANGE) &&
			cookie_len > 0)
			{
			memcpy(s->d1->rcvd_cookie, p, cookie_len);

			if ( s->ctx->app_verify_cookie_cb != NULL)
				{
				if ( s->ctx->app_verify_cookie_cb(s, s->d1->rcvd_cookie,
					cookie_len) == 0)
					{
					al=SSL_AD_HANDSHAKE_FAILURE;
					SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO, 
						SSL_R_COOKIE_MISMATCH);
					goto f_err;
					}
				/* else cookie verification succeeded */
				}
			else if ( memcmp(s->d1->rcvd_cookie, s->d1->cookie, 
						  s->d1->cookie_len) != 0) /* default verification */
				{
					al=SSL_AD_HANDSHAKE_FAILURE;
					SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO, 
						SSL_R_COOKIE_MISMATCH);
					goto f_err;
				}
			}

		p += cookie_len;
		}

	n2s(p,i);
	if ((i == 0) && (j != 0))
		{
		/* we need a cipher if we are not resuming a session */
		al=SSL_AD_ILLEGAL_PARAMETER;
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_NO_CIPHERS_SPECIFIED);
		goto f_err;
		}
	if ((p+i) >= (d+n))
		{
		/* not enough data */
		al=SSL_AD_DECODE_ERROR;
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_LENGTH_MISMATCH);
		goto f_err;
		}
	if ((i > 0) && (ssl_bytes_to_cipher_list(s,p,i,&(ciphers))
		== NULL))
		{
		goto err;
		}
	p+=i;

	/* If it is a hit, check that the cipher is in the list */
	if ((s->hit) && (i > 0))
		{
		j=0;
		id=s->session->cipher->id;

#ifdef CIPHER_DEBUG
		printf("client sent %d ciphers\n",sk_num(ciphers));
#endif
		for (i=0; i<sk_SSL_CIPHER_num(ciphers); i++)
			{
			c=sk_SSL_CIPHER_value(ciphers,i);
#ifdef CIPHER_DEBUG
			printf("client [%2d of %2d]:%s\n",
				i,sk_num(ciphers),SSL_CIPHER_get_name(c));
#endif
			if (c->id == id)
				{
				j=1;
				break;
				}
			}
		if (j == 0)
			{
			if ((s->options & SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG) && (sk_SSL_CIPHER_num(ciphers) == 1))
				{
				/* Very bad for multi-threading.... */
				s->session->cipher=sk_SSL_CIPHER_value(ciphers, 0);
				}
			else
				{
				/* we need to have the cipher in the cipher
				 * list if we are asked to reuse it */
				al=SSL_AD_ILLEGAL_PARAMETER;
				SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_REQUIRED_CIPHER_MISSING);
				goto f_err;
				}
			}
		}

	/* compression */
	i= *(p++);
	if ((p+i) > (d+n))
		{
		/* not enough data */
		al=SSL_AD_DECODE_ERROR;
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_LENGTH_MISMATCH);
		goto f_err;
		}
	q=p;
	for (j=0; j<i; j++)
		{
		if (p[j] == 0) break;
		}

	p+=i;
	if (j >= i)
		{
		/* no compress */
		al=SSL_AD_DECODE_ERROR;
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_NO_COMPRESSION_SPECIFIED);
		goto f_err;
		}

#ifndef OPENSSL_NO_TLSEXT
	/* TLS extensions*/
	if (s->version > SSL3_VERSION)
		{
		if (!ssl_parse_clienthello_tlsext(s,&p,d,n, &al))
			{
			/* 'al' set by ssl_parse_clienthello_tlsext */
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_PARSE_TLS_EXT);
			goto f_err;
			}
		}
		if (ssl_check_tlsext(s,1) <= 0) {
			SSLerr(SSL_F_SSL3_ACCEPT,SSL_R_CLIENTHELLO_TLS_EXT);
			goto err;
		}
#endif

	/* Worst case, we will use the NULL compression, but if we have other
	 * options, we will now look for them.  We have i-1 compression
	 * algorithms from the client, starting at q. */
	s->s3->tmp.new_compression=NULL;
#ifndef OPENSSL_NO_COMP
	if (!(s->options & SSL_OP_NO_COMPRESSION) && s->ctx->comp_methods)
		{ /* See if we have a match */
		int m,nn,o,v,done=0;

		nn=sk_SSL_COMP_num(s->ctx->comp_methods);
		for (m=0; m<nn; m++)
			{
			comp=sk_SSL_COMP_value(s->ctx->comp_methods,m);
			v=comp->id;
			for (o=0; o<i; o++)
				{
				if (v == q[o])
					{
					done=1;
					break;
					}
				}
			if (done) break;
			}
		if (done)
			s->s3->tmp.new_compression=comp;
		else
			comp=NULL;
		}
#endif

	/* Given s->session->ciphers and SSL_get_ciphers, we must
	 * pick a cipher */

	if (!s->hit)
		{
#ifdef OPENSSL_NO_COMP
		s->session->compress_meth=0;
#else
		s->session->compress_meth=(comp == NULL)?0:comp->id;
#endif
		if (s->session->ciphers != NULL)
			sk_SSL_CIPHER_free(s->session->ciphers);
		s->session->ciphers=ciphers;
		if (ciphers == NULL)
			{
			al=SSL_AD_ILLEGAL_PARAMETER;
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_NO_CIPHERS_PASSED);
			goto f_err;
			}
		ciphers=NULL;
		c=ssl3_choose_cipher(s,s->session->ciphers,
				     SSL_get_ciphers(s));

		if (c == NULL)
			{
			al=SSL_AD_HANDSHAKE_FAILURE;
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_NO_SHARED_CIPHER);
			goto f_err;
			}
		s->s3->tmp.new_cipher=c;
		}
	else
		{
		/* Session-id reuse */
#ifdef REUSE_CIPHER_BUG
		STACK_OF(SSL_CIPHER) *sk;
		SSL_CIPHER *nc=NULL;
		SSL_CIPHER *ec=NULL;

		if (s->options & SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG)
			{
			sk=s->session->ciphers;
			for (i=0; i<sk_SSL_CIPHER_num(sk); i++)
				{
				c=sk_SSL_CIPHER_value(sk,i);
				if (c->algorithms & SSL_eNULL)
					nc=c;
				if (SSL_C_IS_EXPORT(c))
					ec=c;
				}
			if (nc != NULL)
				s->s3->tmp.new_cipher=nc;
			else if (ec != NULL)
				s->s3->tmp.new_cipher=ec;
			else
				s->s3->tmp.new_cipher=s->session->cipher;
			}
		else
#endif
		s->s3->tmp.new_cipher=s->session->cipher;
		}
	
	/* we now have the following setup. 
	 * client_random
	 * cipher_list 		- our prefered list of ciphers
	 * ciphers 		- the clients prefered list of ciphers
	 * compression		- basically ignored right now
	 * ssl version is set	- sslv3
	 * s->session		- The ssl session has been setup.
	 * s->hit		- session reuse flag
	 * s->tmp.new_cipher	- the new cipher to use.
	 */

	ret=1;
	if (0)
		{
f_err:
		ssl3_send_alert(s,SSL3_AL_FATAL,al);
		}
err:
	if (ciphers != NULL) sk_SSL_CIPHER_free(ciphers);
	return(ret);
	}