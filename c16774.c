static int tls_construct_cke_dhe(SSL *s, unsigned char **p, int *len, int *al)
{
#ifndef OPENSSL_NO_DH
    DH *dh_clnt = NULL;
    const BIGNUM *pub_key;
    EVP_PKEY *ckey = NULL, *skey = NULL;

    skey = s->s3->peer_tmp;
    if (skey == NULL) {
        SSLerr(SSL_F_TLS_CONSTRUCT_CKE_DHE, ERR_R_INTERNAL_ERROR);
        return 0;
    }
    ckey = ssl_generate_pkey(skey);
    dh_clnt = EVP_PKEY_get0_DH(ckey);

    if (dh_clnt == NULL || ssl_derive(s, ckey, skey) == 0) {
        SSLerr(SSL_F_TLS_CONSTRUCT_CKE_DHE, ERR_R_INTERNAL_ERROR);
        EVP_PKEY_free(ckey);
        return 0;
    }

    /* send off the data */
    DH_get0_key(dh_clnt, &pub_key, NULL);
    *len = BN_num_bytes(pub_key);
    s2n(*len, *p);
    BN_bn2bin(pub_key, *p);
    *len += 2;
    EVP_PKEY_free(ckey);

    return 1;
#else
    SSLerr(SSL_F_TLS_CONSTRUCT_CKE_DHE, ERR_R_INTERNAL_ERROR);
    *al = SSL_AD_INTERNAL_ERROR;
    return 0;
#endif
}