X509_REQ *X509_to_X509_REQ(X509 *x, EVP_PKEY *pkey, const EVP_MD *md)
{
    X509_REQ *ret;
    X509_REQ_INFO *ri;
    int i;
    EVP_PKEY *pktmp;

    ret = X509_REQ_new();
    if (ret == NULL) {
        X509err(X509_F_X509_TO_X509_REQ, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    ri = ret->req_info;

    ri->version->length = 1;
    ri->version->data = (unsigned char *)OPENSSL_malloc(1);
    if (ri->version->data == NULL)
        goto err;
    ri->version->data[0] = 0;   /* version == 0 */

    if (!X509_REQ_set_subject_name(ret, X509_get_subject_name(x)))
        goto err;

    pktmp = X509_get_pubkey(x);
    i = X509_REQ_set_pubkey(ret, pktmp);
    EVP_PKEY_free(pktmp);
    if (!i)
        goto err;

    if (pkey != NULL) {
        if (!X509_REQ_sign(ret, pkey, md))
            goto err;
    }
    return (ret);
 err:
    X509_REQ_free(ret);
    return (NULL);
}