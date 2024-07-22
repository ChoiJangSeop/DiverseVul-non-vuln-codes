int ssl_parse_clienthello_use_srtp_ext(SSL *s, unsigned char *d, int len,int *al)
	{
	SRTP_PROTECTION_PROFILE *cprof,*sprof;
	STACK_OF(SRTP_PROTECTION_PROFILE) *clnt=0,*srvr;
        int ct;
        int mki_len;
	int i,j;
	int id;
	int ret;

         /* Length value + the MKI length */
        if(len < 3)
		{            
		SSLerr(SSL_F_SSL_PARSE_CLIENTHELLO_USE_SRTP_EXT,SSL_R_BAD_SRTP_PROTECTION_PROFILE_LIST);
		*al=SSL_AD_DECODE_ERROR;
		return 1;
                }

        /* Pull off the length of the cipher suite list */
        n2s(d, ct);
        len -= 2;
        
        /* Check that it is even */
	if(ct%2)
		{
		SSLerr(SSL_F_SSL_PARSE_CLIENTHELLO_USE_SRTP_EXT,SSL_R_BAD_SRTP_PROTECTION_PROFILE_LIST);
		*al=SSL_AD_DECODE_ERROR;
		return 1;
		}
        
        /* Check that lengths are consistent */
	if(len < (ct + 1)) 
		{
		SSLerr(SSL_F_SSL_PARSE_CLIENTHELLO_USE_SRTP_EXT,SSL_R_BAD_SRTP_PROTECTION_PROFILE_LIST);
		*al=SSL_AD_DECODE_ERROR;
		return 1;
		}

        
	clnt=sk_SRTP_PROTECTION_PROFILE_new_null();

	while(ct)
		{
		n2s(d,id);
		ct-=2;
                len-=2;

		if(!find_profile_by_num(id,&cprof))
			{
			sk_SRTP_PROTECTION_PROFILE_push(clnt,cprof);
			}
		else
			{
			; /* Ignore */
			}
		}

        /* Now extract the MKI value as a sanity check, but discard it for now */
        mki_len = *d;
        d++; len--;

        if (mki_len != len)
		{
		SSLerr(SSL_F_SSL_PARSE_CLIENTHELLO_USE_SRTP_EXT,SSL_R_BAD_SRTP_MKI_VALUE);
		*al=SSL_AD_DECODE_ERROR;
		return 1;
		}

	srvr=SSL_get_srtp_profiles(s);

	/* Pick our most preferred profile. If no profiles have been
	 configured then the outer loop doesn't run 
	 (sk_SRTP_PROTECTION_PROFILE_num() = -1)
	 and so we just return without doing anything */
	for(i=0;i<sk_SRTP_PROTECTION_PROFILE_num(srvr);i++)
		{
		sprof=sk_SRTP_PROTECTION_PROFILE_value(srvr,i);

		for(j=0;j<sk_SRTP_PROTECTION_PROFILE_num(clnt);j++)
			{
			cprof=sk_SRTP_PROTECTION_PROFILE_value(clnt,j);
            
			if(cprof->id==sprof->id)
				{
				s->srtp_profile=sprof;
				*al=0;
				ret=0;
				goto done;
				}
			}
		}

	ret=0;
    
done:
	if(clnt) sk_SRTP_PROTECTION_PROFILE_free(clnt);

	return ret;
	}