int ssl2_generate_key_material(SSL *s)
{
    unsigned int i;
    EVP_MD_CTX ctx;
    unsigned char *km;
    unsigned char c = '0';
    const EVP_MD *md5;
    int md_size;

    md5 = EVP_md5();

# ifdef CHARSET_EBCDIC
    c = os_toascii['0'];        /* Must be an ASCII '0', not EBCDIC '0', see
                                 * SSLv2 docu */
# endif
    EVP_MD_CTX_init(&ctx);
    km = s->s2->key_material;

    if (s->session->master_key_length < 0 ||
        s->session->master_key_length > (int)sizeof(s->session->master_key)) {
        SSLerr(SSL_F_SSL2_GENERATE_KEY_MATERIAL, ERR_R_INTERNAL_ERROR);
        return 0;
    }
    md_size = EVP_MD_size(md5);
    if (md_size < 0)
        return 0;
    for (i = 0; i < s->s2->key_material_length; i += md_size) {
        if (((km - s->s2->key_material) + md_size) >
            (int)sizeof(s->s2->key_material)) {
            /*
             * EVP_DigestFinal_ex() below would write beyond buffer
             */
            SSLerr(SSL_F_SSL2_GENERATE_KEY_MATERIAL, ERR_R_INTERNAL_ERROR);
            return 0;
        }

        EVP_DigestInit_ex(&ctx, md5, NULL);

        OPENSSL_assert(s->session->master_key_length >= 0
                       && s->session->master_key_length
                       < (int)sizeof(s->session->master_key));
        EVP_DigestUpdate(&ctx, s->session->master_key,
                         s->session->master_key_length);
        EVP_DigestUpdate(&ctx, &c, 1);
        c++;
        EVP_DigestUpdate(&ctx, s->s2->challenge, s->s2->challenge_length);
        EVP_DigestUpdate(&ctx, s->s2->conn_id, s->s2->conn_id_length);
        EVP_DigestFinal_ex(&ctx, km, NULL);
        km += md_size;
    }

    EVP_MD_CTX_cleanup(&ctx);
    return 1;
}