static int pkey_GOST_ECcp_decrypt(EVP_PKEY_CTX *pctx, unsigned char *key,
                           size_t *key_len, const unsigned char *in,
                           size_t in_len)
{
    const unsigned char *p = in;
    EVP_PKEY *priv = EVP_PKEY_CTX_get0_pkey(pctx);
    GOST_KEY_TRANSPORT *gkt = NULL;
    int ret = 0;
    unsigned char wrappedKey[44];
    unsigned char sharedKey[32];
    gost_ctx ctx;
    const struct gost_cipher_info *param = NULL;
    EVP_PKEY *eph_key = NULL, *peerkey = NULL;
    int dgst_nid = NID_undef;

    if (!key) {
        *key_len = 32;
        return 1;
    }
    gkt = d2i_GOST_KEY_TRANSPORT(NULL, (const unsigned char **)&p, in_len);
    if (!gkt) {
        GOSTerr(GOST_F_PKEY_GOST_ECCP_DECRYPT,
                GOST_R_ERROR_PARSING_KEY_TRANSPORT_INFO);
        return -1;
    }

    /* If key transport structure contains public key, use it */
    eph_key = X509_PUBKEY_get(gkt->key_agreement_info->ephem_key);
    if (eph_key) {
        if (EVP_PKEY_derive_set_peer(pctx, eph_key) <= 0) {
            GOSTerr(GOST_F_PKEY_GOST_ECCP_DECRYPT,
                    GOST_R_INCOMPATIBLE_PEER_KEY);
            goto err;
        }
    } else {
        /* Set control "public key from client certificate used" */
        if (EVP_PKEY_CTX_ctrl(pctx, -1, -1, EVP_PKEY_CTRL_PEER_KEY, 3, NULL)
            <= 0) {
            GOSTerr(GOST_F_PKEY_GOST_ECCP_DECRYPT, GOST_R_CTRL_CALL_FAILED);
            goto err;
        }
    }
    peerkey = EVP_PKEY_CTX_get0_peerkey(pctx);
    if (!peerkey) {
        GOSTerr(GOST_F_PKEY_GOST_ECCP_DECRYPT, GOST_R_NO_PEER_KEY);
        goto err;
    }

    param = get_encryption_params(gkt->key_agreement_info->cipher);
    if (!param) {
        goto err;
    }

    gost_init(&ctx, param->sblock);
    OPENSSL_assert(gkt->key_agreement_info->eph_iv->length == 8);
    memcpy(wrappedKey, gkt->key_agreement_info->eph_iv->data, 8);
    OPENSSL_assert(gkt->key_info->encrypted_key->length == 32);
    memcpy(wrappedKey + 8, gkt->key_info->encrypted_key->data, 32);
    OPENSSL_assert(gkt->key_info->imit->length == 4);
    memcpy(wrappedKey + 40, gkt->key_info->imit->data, 4);

    EVP_PKEY_get_default_digest_nid(priv, &dgst_nid);
    if (dgst_nid == NID_id_GostR3411_2012_512)
        dgst_nid = NID_id_GostR3411_2012_256;

    if (!VKO_compute_key(sharedKey,
                         EC_KEY_get0_public_key(EVP_PKEY_get0(peerkey)),
                         EVP_PKEY_get0(priv), wrappedKey, 8, dgst_nid)) {
        GOSTerr(GOST_F_PKEY_GOST_ECCP_DECRYPT,
                GOST_R_ERROR_COMPUTING_SHARED_KEY);
        goto err;
    }
    if (!keyUnwrapCryptoPro(&ctx, sharedKey, wrappedKey, key)) {
        GOSTerr(GOST_F_PKEY_GOST_ECCP_DECRYPT,
                GOST_R_ERROR_COMPUTING_SHARED_KEY);
        goto err;
    }

    ret = 1;
 err:
    OPENSSL_cleanse(sharedKey, sizeof(sharedKey));
    EVP_PKEY_free(eph_key);
    GOST_KEY_TRANSPORT_free(gkt);
    return ret;
}