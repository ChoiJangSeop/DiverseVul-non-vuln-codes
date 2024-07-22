static int ssl23_get_server_hello(SSL *s)
	{
	char buf[8];
	unsigned char *p;
	int i;
	int n;

	n=ssl23_read_bytes(s,7);

	if (n != 7) return(n);
	p=s->packet;

	memcpy(buf,p,n);

	if ((p[0] & 0x80) && (p[2] == SSL2_MT_SERVER_HELLO) &&
		(p[5] == 0x00) && (p[6] == 0x02))
		{
#ifdef OPENSSL_NO_SSL2
		SSLerr(SSL_F_SSL23_GET_SERVER_HELLO,SSL_R_UNSUPPORTED_PROTOCOL);
		goto err;
#else
		/* we are talking sslv2 */
		/* we need to clean up the SSLv3 setup and put in the
		 * sslv2 stuff. */
		int ch_len;

		if (s->options & SSL_OP_NO_SSLv2)
			{
			SSLerr(SSL_F_SSL23_GET_SERVER_HELLO,SSL_R_UNSUPPORTED_PROTOCOL);
			goto err;
			}
		if (s->s2 == NULL)
			{
			if (!ssl2_new(s))
				goto err;
			}
		else
			ssl2_clear(s);

		if (s->options & SSL_OP_NETSCAPE_CHALLENGE_BUG)
			ch_len=SSL2_CHALLENGE_LENGTH;
		else
			ch_len=SSL2_MAX_CHALLENGE_LENGTH;

		/* write out sslv2 challenge */
		/* Note that ch_len must be <= SSL3_RANDOM_SIZE (32), because
		   it is one of SSL2_MAX_CHALLENGE_LENGTH (32) or
		   SSL2_MAX_CHALLENGE_LENGTH (16), but leave the check in for
		   futurproofing */
		i=(SSL3_RANDOM_SIZE < ch_len)
			?SSL3_RANDOM_SIZE:ch_len;
		s->s2->challenge_length=i;
		memcpy(s->s2->challenge,
			&(s->s3->client_random[SSL3_RANDOM_SIZE-i]),i);

		if (s->s3 != NULL) ssl3_free(s);

		if (!BUF_MEM_grow_clean(s->init_buf,
			SSL2_MAX_RECORD_LENGTH_3_BYTE_HEADER))
			{
			SSLerr(SSL_F_SSL23_GET_SERVER_HELLO,ERR_R_BUF_LIB);
			goto err;
			}

		s->state=SSL2_ST_GET_SERVER_HELLO_A;
		if (!(s->client_version == SSL2_VERSION))
			/* use special padding (SSL 3.0 draft/RFC 2246, App. E.2) */
			s->s2->ssl2_rollback=1;

		/* setup the 7 bytes we have read so we get them from
		 * the sslv2 buffer */
		s->rstate=SSL_ST_READ_HEADER;
		s->packet_length=n;
		s->packet= &(s->s2->rbuf[0]);
		memcpy(s->packet,buf,n);
		s->s2->rbuf_left=n;
		s->s2->rbuf_offs=0;

		/* we have already written one */
		s->s2->write_sequence=1;

		s->method=SSLv2_client_method();
		s->handshake_func=s->method->ssl_connect;
#endif
		}
	else if (p[1] == SSL3_VERSION_MAJOR &&
	         p[2] <= TLS1_2_VERSION_MINOR &&
	         ((p[0] == SSL3_RT_HANDSHAKE && p[5] == SSL3_MT_SERVER_HELLO) ||
	          (p[0] == SSL3_RT_ALERT && p[3] == 0 && p[4] == 2)))
		{
		/* we have sslv3 or tls1 (server hello or alert) */

		if ((p[2] == SSL3_VERSION_MINOR) &&
			!(s->options & SSL_OP_NO_SSLv3))
			{
#ifdef OPENSSL_FIPS
			if(FIPS_mode())
				{
				SSLerr(SSL_F_SSL23_GET_SERVER_HELLO,
					SSL_R_ONLY_TLS_ALLOWED_IN_FIPS_MODE);
				goto err;
				}
#endif
			s->version=SSL3_VERSION;
			s->method=SSLv3_client_method();
			}
		else if ((p[2] == TLS1_VERSION_MINOR) &&
			!(s->options & SSL_OP_NO_TLSv1))
			{
			s->version=TLS1_VERSION;
			s->method=TLSv1_client_method();
			}
		else if ((p[2] == TLS1_1_VERSION_MINOR) &&
			!(s->options & SSL_OP_NO_TLSv1_1))
			{
			s->version=TLS1_1_VERSION;
			s->method=TLSv1_1_client_method();
			}
		else if ((p[2] == TLS1_2_VERSION_MINOR) &&
			!(s->options & SSL_OP_NO_TLSv1_2))
			{
			s->version=TLS1_2_VERSION;
			s->method=TLSv1_2_client_method();
			}
		else
			{
			SSLerr(SSL_F_SSL23_GET_SERVER_HELLO,SSL_R_UNSUPPORTED_PROTOCOL);
			goto err;
			}

		if (!ssl_security(s, SSL_SECOP_VERSION, 0, s->version, NULL))
			{
			SSLerr(SSL_F_SSL23_GET_SERVER_HELLO,SSL_R_VERSION_TOO_LOW);
			goto err;
			}

		if (p[0] == SSL3_RT_ALERT && p[5] != SSL3_AL_WARNING)
			{
			/* fatal alert */

			void (*cb)(const SSL *ssl,int type,int val)=NULL;
			int j;

			if (s->info_callback != NULL)
				cb=s->info_callback;
			else if (s->ctx->info_callback != NULL)
				cb=s->ctx->info_callback;
 
			i=p[5];
			if (cb != NULL)
				{
				j=(i<<8)|p[6];
				cb(s,SSL_CB_READ_ALERT,j);
				}
			
			if (s->msg_callback)
				{
				s->msg_callback(0, s->version, SSL3_RT_HEADER, p, 5, s, s->msg_callback_arg);
				s->msg_callback(0, s->version, SSL3_RT_ALERT, p+5, 2, s, s->msg_callback_arg);
				}

			s->rwstate=SSL_NOTHING;
			SSLerr(SSL_F_SSL23_GET_SERVER_HELLO,SSL_AD_REASON_OFFSET+p[6]);
			goto err;
			}

		if (!ssl_init_wbio_buffer(s,1)) goto err;

		/* we are in this state */
		s->state=SSL3_ST_CR_SRVR_HELLO_A;

		/* put the 7 bytes we have read into the input buffer
		 * for SSLv3 */
		s->rstate=SSL_ST_READ_HEADER;
		s->packet_length=n;
		if (s->s3->rbuf.buf == NULL)
			if (!ssl3_setup_read_buffer(s))
				goto err;
		s->packet= &(s->s3->rbuf.buf[0]);
		memcpy(s->packet,buf,n);
		s->s3->rbuf.left=n;
		s->s3->rbuf.offset=0;

		s->handshake_func=s->method->ssl_connect;
		}
	else
		{
		SSLerr(SSL_F_SSL23_GET_SERVER_HELLO,SSL_R_UNKNOWN_PROTOCOL);
		goto err;
		}
	s->init_num=0;

	/* Since, if we are sending a ssl23 client hello, we are not
	 * reusing a session-id */
	if (!ssl_get_new_session(s,0))
		goto err;

	return(SSL_connect(s));
err:
	return(-1);
	}