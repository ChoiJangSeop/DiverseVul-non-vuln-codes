static int tls_construct_cke_ecdhe(SSL *s, unsigned char **p, int *len, int *al)
{
#ifndef OPENSSL_NO_EC
    unsigned char *encodedPoint = NULL;
    int encoded_pt_len = 0;
    EVP_PKEY *ckey = NULL, *skey = NULL;

    skey = s->s3->peer_tmp;
    if (skey == NULL) {
        SSLerr(SSL_F_TLS_CONSTRUCT_CKE_ECDHE, ERR_R_INTERNAL_ERROR);
        return 0;
    }

    ckey = ssl_generate_pkey(skey);

    if (ssl_derive(s, ckey, skey) == 0) {
        SSLerr(SSL_F_TLS_CONSTRUCT_CKE_ECDHE, ERR_R_EVP_LIB);
        goto err;
    }

    /* Generate encoding of client key */
    encoded_pt_len = EVP_PKEY_get1_tls_encodedpoint(ckey, &encodedPoint);

    if (encoded_pt_len == 0) {
        SSLerr(SSL_F_TLS_CONSTRUCT_CKE_ECDHE, ERR_R_EC_LIB);
        goto err;
    }

    EVP_PKEY_free(ckey);
    ckey = NULL;

    *len = encoded_pt_len;

    /* length of encoded point */
    **p = *len;
    *p += 1;
    /* copy the point */
    memcpy(*p, encodedPoint, *len);
    /* increment len to account for length field */
    *len += 1;

    OPENSSL_free(encodedPoint);

    return 1;
 err:
    EVP_PKEY_free(ckey);
    return 0;
#else
    SSLerr(SSL_F_TLS_CONSTRUCT_CKE_ECDHE, ERR_R_INTERNAL_ERROR);
    *al = SSL_AD_INTERNAL_ERROR;
    return 0;
#endif
}