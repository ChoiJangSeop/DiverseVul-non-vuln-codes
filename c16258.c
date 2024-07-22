static int sv_body(char *hostname, int s, unsigned char *context)
	{
	char *buf=NULL;
	fd_set readfds;
	int ret=1,width;
	int k,i;
	unsigned long l;
	SSL *con=NULL;
	BIO *sbio;
#ifndef OPENSSL_NO_KRB5
	KSSL_CTX *kctx;
#endif
	struct timeval timeout;
#if defined(OPENSSL_SYS_WINDOWS) || defined(OPENSSL_SYS_MSDOS) || defined(OPENSSL_SYS_NETWARE) || defined(OPENSSL_SYS_BEOS_R5)
	struct timeval tv;
#else
	struct timeval *timeoutp;
#endif

	if ((buf=OPENSSL_malloc(bufsize)) == NULL)
		{
		BIO_printf(bio_err,"out of memory\n");
		goto err;
		}
#ifdef FIONBIO	
	if (s_nbio)
		{
		unsigned long sl=1;

		if (!s_quiet)
			BIO_printf(bio_err,"turning on non blocking io\n");
		if (BIO_socket_ioctl(s,FIONBIO,&sl) < 0)
			ERR_print_errors(bio_err);
		}
#endif

	if (con == NULL) {
		con=SSL_new(ctx);
#ifndef OPENSSL_NO_TLSEXT
	if (s_tlsextdebug)
		{
		SSL_set_tlsext_debug_callback(con, tlsext_cb);
		SSL_set_tlsext_debug_arg(con, bio_s_out);
		}
	if (s_tlsextstatus)
		{
		SSL_CTX_set_tlsext_status_cb(ctx, cert_status_cb);
		tlscstatp.err = bio_err;
		SSL_CTX_set_tlsext_status_arg(ctx, &tlscstatp);
		}
#endif
#ifndef OPENSSL_NO_KRB5
		if ((kctx = kssl_ctx_new()) != NULL)
                        {
			SSL_set0_kssl_ctx(con, kctx);
                        kssl_ctx_setstring(kctx, KSSL_SERVICE, KRB5SVC);
                        kssl_ctx_setstring(kctx, KSSL_KEYTAB, KRB5KEYTAB);
                        }
#endif	/* OPENSSL_NO_KRB5 */
		if(context)
		      SSL_set_session_id_context(con, context,
						 strlen((char *)context));
	}
	SSL_clear(con);
#if 0
#ifdef TLSEXT_TYPE_opaque_prf_input
	SSL_set_tlsext_opaque_prf_input(con, "Test server", 11);
#endif
#endif

	if (SSL_version(con) == DTLS1_VERSION)
		{

		sbio=BIO_new_dgram(s,BIO_NOCLOSE);

		if (enable_timeouts)
			{
			timeout.tv_sec = 0;
			timeout.tv_usec = DGRAM_RCV_TIMEOUT;
			BIO_ctrl(sbio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);
			
			timeout.tv_sec = 0;
			timeout.tv_usec = DGRAM_SND_TIMEOUT;
			BIO_ctrl(sbio, BIO_CTRL_DGRAM_SET_SEND_TIMEOUT, 0, &timeout);
			}

		if (socket_mtu > 28)
			{
			SSL_set_options(con, SSL_OP_NO_QUERY_MTU);
			SSL_set_mtu(con, socket_mtu - 28);
			}
		else
			/* want to do MTU discovery */
			BIO_ctrl(sbio, BIO_CTRL_DGRAM_MTU_DISCOVER, 0, NULL);

        /* turn on cookie exchange */
        SSL_set_options(con, SSL_OP_COOKIE_EXCHANGE);
		}
	else
		sbio=BIO_new_socket(s,BIO_NOCLOSE);

	if (s_nbio_test)
		{
		BIO *test;

		test=BIO_new(BIO_f_nbio_test());
		sbio=BIO_push(test,sbio);
		}
#ifndef OPENSSL_NO_JPAKE
	if(jpake_secret)
		jpake_server_auth(bio_s_out, sbio, jpake_secret);
#endif

	SSL_set_bio(con,sbio,sbio);
	SSL_set_accept_state(con);
	/* SSL_set_fd(con,s); */

	if (s_debug)
		{
		SSL_set_debug(con, 1);
		BIO_set_callback(SSL_get_rbio(con),bio_dump_callback);
		BIO_set_callback_arg(SSL_get_rbio(con),(char *)bio_s_out);
		}
	if (s_msg)
		{
		SSL_set_msg_callback(con, msg_cb);
		SSL_set_msg_callback_arg(con, bio_s_out);
		}
#ifndef OPENSSL_NO_TLSEXT
	if (s_tlsextdebug)
		{
		SSL_set_tlsext_debug_callback(con, tlsext_cb);
		SSL_set_tlsext_debug_arg(con, bio_s_out);
		}
#endif

	width=s+1;
	for (;;)
		{
		int read_from_terminal;
		int read_from_sslcon;

		read_from_terminal = 0;
		read_from_sslcon = SSL_pending(con);

		if (!read_from_sslcon)
			{
			FD_ZERO(&readfds);
#if !defined(OPENSSL_SYS_WINDOWS) && !defined(OPENSSL_SYS_MSDOS) && !defined(OPENSSL_SYS_NETWARE) && !defined(OPENSSL_SYS_BEOS_R5)
			openssl_fdset(fileno(stdin),&readfds);
#endif
			openssl_fdset(s,&readfds);
			/* Note: under VMS with SOCKETSHR the second parameter is
			 * currently of type (int *) whereas under other systems
			 * it is (void *) if you don't have a cast it will choke
			 * the compiler: if you do have a cast then you can either
			 * go for (int *) or (void *).
			 */
#if defined(OPENSSL_SYS_WINDOWS) || defined(OPENSSL_SYS_MSDOS) || defined(OPENSSL_SYS_NETWARE)
                        /* Under DOS (non-djgpp) and Windows we can't select on stdin: only
			 * on sockets. As a workaround we timeout the select every
			 * second and check for any keypress. In a proper Windows
			 * application we wouldn't do this because it is inefficient.
			 */
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			i=select(width,(void *)&readfds,NULL,NULL,&tv);
			if((i < 0) || (!i && !_kbhit() ) )continue;
			if(_kbhit())
				read_from_terminal = 1;
#elif defined(OPENSSL_SYS_BEOS_R5)
			/* Under BeOS-R5 the situation is similar to DOS */
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			(void)fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);
			i=select(width,(void *)&readfds,NULL,NULL,&tv);
			if ((i < 0) || (!i && read(fileno(stdin), buf, 0) < 0))
				continue;
			if (read(fileno(stdin), buf, 0) >= 0)
				read_from_terminal = 1;
			(void)fcntl(fileno(stdin), F_SETFL, 0);
#else
			if ((SSL_version(con) == DTLS1_VERSION) &&
				DTLSv1_get_timeout(con, &timeout))
				timeoutp = &timeout;
			else
				timeoutp = NULL;

			i=select(width,(void *)&readfds,NULL,NULL,timeoutp);

			if ((SSL_version(con) == DTLS1_VERSION) && DTLSv1_handle_timeout(con) > 0)
				{
				BIO_printf(bio_err,"TIMEOUT occured\n");
				}

			if (i <= 0) continue;
			if (FD_ISSET(fileno(stdin),&readfds))
				read_from_terminal = 1;
#endif
			if (FD_ISSET(s,&readfds))
				read_from_sslcon = 1;
			}
		if (read_from_terminal)
			{
			if (s_crlf)
				{
				int j, lf_num;

				i=raw_read_stdin(buf, bufsize/2);
				lf_num = 0;
				/* both loops are skipped when i <= 0 */
				for (j = 0; j < i; j++)
					if (buf[j] == '\n')
						lf_num++;
				for (j = i-1; j >= 0; j--)
					{
					buf[j+lf_num] = buf[j];
					if (buf[j] == '\n')
						{
						lf_num--;
						i++;
						buf[j+lf_num] = '\r';
						}
					}
				assert(lf_num == 0);
				}
			else
				i=raw_read_stdin(buf,bufsize);
			if (!s_quiet)
				{
				if ((i <= 0) || (buf[0] == 'Q'))
					{
					BIO_printf(bio_s_out,"DONE\n");
					SHUTDOWN(s);
					close_accept_socket();
					ret= -11;
					goto err;
					}
				if ((i <= 0) || (buf[0] == 'q'))
					{
					BIO_printf(bio_s_out,"DONE\n");
					if (SSL_version(con) != DTLS1_VERSION)
                        SHUTDOWN(s);
	/*				close_accept_socket();
					ret= -11;*/
					goto err;
					}

				if ((buf[0] == 'r') && 
					((buf[1] == '\n') || (buf[1] == '\r')))
					{
					SSL_renegotiate(con);
					i=SSL_do_handshake(con);
					printf("SSL_do_handshake -> %d\n",i);
					i=0; /*13; */
					continue;
					/* strcpy_s(buf, sizeof(buf), "server side RE-NEGOTIATE\n"); */
					}
				if ((buf[0] == 'R') &&
					((buf[1] == '\n') || (buf[1] == '\r')))
					{
					SSL_set_verify(con,
						SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE,NULL);
					SSL_renegotiate(con);
					i=SSL_do_handshake(con);
					printf("SSL_do_handshake -> %d\n",i);
					i=0; /* 13; */
					continue;
					/* strcpy_s(buf, sizeof(buf), "server side RE-NEGOTIATE asking for client cert\n"); */
					}
				if (buf[0] == 'P')
					{
					static const char *str="Lets print some clear text\n";
					BIO_write(SSL_get_wbio(con),str,strlen(str));
					}
				if (buf[0] == 'S')
					{
					print_stats(bio_s_out,SSL_get_SSL_CTX(con));
					}
				}
#ifdef CHARSET_EBCDIC
			ebcdic2ascii(buf,buf,i);
#endif
			l=k=0;
			for (;;)
				{
				/* should do a select for the write */
#ifdef RENEG
{ static count=0; if (++count == 100) { count=0; SSL_renegotiate(con); } }
#endif
				k=SSL_write(con,&(buf[l]),(unsigned int)i);
				while (SSL_get_error(con,k) == SSL_ERROR_WANT_X509_LOOKUP)
					{
					BIO_printf(bio_s_out,"LOOKUP renego during write\n");
					srp_callback_parm.user = SRP_VBASE_get_by_user(srp_callback_parm.vb, srp_callback_parm.login); 
					if (srp_callback_parm.user) 
						BIO_printf(bio_s_out,"LOOKUP done %s\n",srp_callback_parm.user->info);
					else 
						BIO_printf(bio_s_out,"LOOKUP not successful\n");
						k=SSL_write(con,&(buf[l]),(unsigned int)i);
					}
				switch (SSL_get_error(con,k))
					{
				case SSL_ERROR_NONE:
					break;
				case SSL_ERROR_WANT_WRITE:
				case SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_X509_LOOKUP:
					BIO_printf(bio_s_out,"Write BLOCK\n");
					break;
				case SSL_ERROR_SYSCALL:
				case SSL_ERROR_SSL:
					BIO_printf(bio_s_out,"ERROR\n");
					ERR_print_errors(bio_err);
					ret=1;
					goto err;
					/* break; */
				case SSL_ERROR_ZERO_RETURN:
					BIO_printf(bio_s_out,"DONE\n");
					ret=1;
					goto err;
					}
				l+=k;
				i-=k;
				if (i <= 0) break;
				}
			}
		if (read_from_sslcon)
			{
			if (!SSL_is_init_finished(con))
				{
				i=init_ssl_connection(con);
				
				if (i < 0)
					{
					ret=0;
					goto err;
					}
				else if (i == 0)
					{
					ret=1;
					goto err;
					}
				}
			else
				{
again:	
				i=SSL_read(con,(char *)buf,bufsize);
				while (SSL_get_error(con,i) == SSL_ERROR_WANT_X509_LOOKUP)
					{
					BIO_printf(bio_s_out,"LOOKUP renego during read\n");
					srp_callback_parm.user = SRP_VBASE_get_by_user(srp_callback_parm.vb, srp_callback_parm.login); 
					if (srp_callback_parm.user) 
						BIO_printf(bio_s_out,"LOOKUP done %s\n",srp_callback_parm.user->info);
					else 
						BIO_printf(bio_s_out,"LOOKUP not successful\n");
					i=SSL_read(con,(char *)buf,bufsize);
					}
				switch (SSL_get_error(con,i))
					{
				case SSL_ERROR_NONE:
#ifdef CHARSET_EBCDIC
					ascii2ebcdic(buf,buf,i);
#endif
					raw_write_stdout(buf,
						(unsigned int)i);
					if (SSL_pending(con)) goto again;
					break;
				case SSL_ERROR_WANT_WRITE:
				case SSL_ERROR_WANT_READ:
					BIO_printf(bio_s_out,"Read BLOCK\n");
					break;
				case SSL_ERROR_SYSCALL:
				case SSL_ERROR_SSL:
					BIO_printf(bio_s_out,"ERROR\n");
					ERR_print_errors(bio_err);
					ret=1;
					goto err;
				case SSL_ERROR_ZERO_RETURN:
					BIO_printf(bio_s_out,"DONE\n");
					ret=1;
					goto err;
					}
				}
			}
		}
err:
	if (con != NULL)
		{
		BIO_printf(bio_s_out,"shutting down SSL\n");
#if 1
		SSL_set_shutdown(con,SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
#else
		SSL_shutdown(con);
#endif
		SSL_free(con);
		}
	BIO_printf(bio_s_out,"CONNECTION CLOSED\n");
	if (buf != NULL)
		{
		OPENSSL_cleanse(buf,bufsize);
		OPENSSL_free(buf);
		}
	if (ret >= 0)
		BIO_printf(bio_s_out,"ACCEPT\n");
	return(ret);
	}