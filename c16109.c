static int init_ssl_connection(SSL *con)
	{
	int i;
	const char *str;
	X509 *peer;
	long verify_error;
	MS_STATIC char buf[BUFSIZ];
#ifndef OPENSSL_NO_KRB5
	char *client_princ;
#endif
#if !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)
	const unsigned char *next_proto_neg;
	unsigned next_proto_neg_len;
#endif
	unsigned char *exportedkeymat;


	i=SSL_accept(con);
#ifndef OPENSSL_NO_SRP
	while (i <= 0 &&  SSL_get_error(con,i) == SSL_ERROR_WANT_X509_LOOKUP) 
		{
			BIO_printf(bio_s_out,"LOOKUP during accept %s\n",srp_callback_parm.login);
			srp_callback_parm.user = SRP_VBASE_get_by_user(srp_callback_parm.vb, srp_callback_parm.login); 
			if (srp_callback_parm.user) 
				BIO_printf(bio_s_out,"LOOKUP done %s\n",srp_callback_parm.user->info);
			else 
				BIO_printf(bio_s_out,"LOOKUP not successful\n");
			i=SSL_accept(con);
		}
#endif
	if (i <= 0)
		{
		if (BIO_sock_should_retry(i))
			{
			BIO_printf(bio_s_out,"DELAY\n");
			return(1);
			}

		BIO_printf(bio_err,"ERROR\n");
		verify_error=SSL_get_verify_result(con);
		if (verify_error != X509_V_OK)
			{
			BIO_printf(bio_err,"verify error:%s\n",
				X509_verify_cert_error_string(verify_error));
			}
		/* Always print any error messages */
		ERR_print_errors(bio_err);
		return(0);
		}

	if (s_brief)
		print_ssl_summary(bio_err, con);

	PEM_write_bio_SSL_SESSION(bio_s_out,SSL_get_session(con));

	peer=SSL_get_peer_certificate(con);
	if (peer != NULL)
		{
		BIO_printf(bio_s_out,"Client certificate\n");
		PEM_write_bio_X509(bio_s_out,peer);
		X509_NAME_oneline(X509_get_subject_name(peer),buf,sizeof buf);
		BIO_printf(bio_s_out,"subject=%s\n",buf);
		X509_NAME_oneline(X509_get_issuer_name(peer),buf,sizeof buf);
		BIO_printf(bio_s_out,"issuer=%s\n",buf);
		X509_free(peer);
		}

	if (SSL_get_shared_ciphers(con,buf,sizeof buf) != NULL)
		BIO_printf(bio_s_out,"Shared ciphers:%s\n",buf);
	str=SSL_CIPHER_get_name(SSL_get_current_cipher(con));
	ssl_print_sigalgs(bio_s_out, con);
	ssl_print_curves(bio_s_out, con, 0);
	BIO_printf(bio_s_out,"CIPHER is %s\n",(str != NULL)?str:"(NONE)");

#if !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)
	SSL_get0_next_proto_negotiated(con, &next_proto_neg, &next_proto_neg_len);
	if (next_proto_neg)
		{
		BIO_printf(bio_s_out,"NEXTPROTO is ");
		BIO_write(bio_s_out, next_proto_neg, next_proto_neg_len);
		BIO_printf(bio_s_out, "\n");
		}
#endif
	{
	SRTP_PROTECTION_PROFILE *srtp_profile
	  = SSL_get_selected_srtp_profile(con);

	if(srtp_profile)
		BIO_printf(bio_s_out,"SRTP Extension negotiated, profile=%s\n",
			   srtp_profile->name);
	}
	if (SSL_cache_hit(con)) BIO_printf(bio_s_out,"Reused session-id\n");
	if (SSL_ctrl(con,SSL_CTRL_GET_FLAGS,0,NULL) &
		TLS1_FLAGS_TLS_PADDING_BUG)
		BIO_printf(bio_s_out,
			   "Peer has incorrect TLSv1 block padding\n");
#ifndef OPENSSL_NO_KRB5
	client_princ = kssl_ctx_get0_client_princ(SSL_get0_kssl_ctx(con));
	if (client_princ != NULL)
		{
		BIO_printf(bio_s_out,"Kerberos peer principal is %s\n",
								client_princ);
		}
#endif /* OPENSSL_NO_KRB5 */
	BIO_printf(bio_s_out, "Secure Renegotiation IS%s supported\n",
		      SSL_get_secure_renegotiation_support(con) ? "" : " NOT");
	if (keymatexportlabel != NULL)
		{
		BIO_printf(bio_s_out, "Keying material exporter:\n");
		BIO_printf(bio_s_out, "    Label: '%s'\n", keymatexportlabel);
		BIO_printf(bio_s_out, "    Length: %i bytes\n",
			   keymatexportlen);
		exportedkeymat = OPENSSL_malloc(keymatexportlen);
		if (exportedkeymat != NULL)
			{
			if (!SSL_export_keying_material(con, exportedkeymat,
						        keymatexportlen,
						        keymatexportlabel,
						        strlen(keymatexportlabel),
						        NULL, 0, 0))
				{
				BIO_printf(bio_s_out, "    Error\n");
				}
			else
				{
				BIO_printf(bio_s_out, "    Keying material: ");
				for (i=0; i<keymatexportlen; i++)
					BIO_printf(bio_s_out, "%02X",
						   exportedkeymat[i]);
				BIO_printf(bio_s_out, "\n");
				}
			OPENSSL_free(exportedkeymat);
			}
		}

	return(1);
	}