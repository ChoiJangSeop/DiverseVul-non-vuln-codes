ASN1_INTEGER *BN_to_ASN1_INTEGER(const BIGNUM *bn, ASN1_INTEGER *ai)
{
    ASN1_INTEGER *ret;
    int len, j;

    if (ai == NULL)
        ret = M_ASN1_INTEGER_new();
    else
        ret = ai;
    if (ret == NULL) {
        ASN1err(ASN1_F_BN_TO_ASN1_INTEGER, ERR_R_NESTED_ASN1_ERROR);
        goto err;
    }
    if (BN_is_negative(bn))
        ret->type = V_ASN1_NEG_INTEGER;
    else
        ret->type = V_ASN1_INTEGER;
    j = BN_num_bits(bn);
    len = ((j == 0) ? 0 : ((j / 8) + 1));
    if (ret->length < len + 4) {
        unsigned char *new_data = OPENSSL_realloc(ret->data, len + 4);
        if (!new_data) {
            ASN1err(ASN1_F_BN_TO_ASN1_INTEGER, ERR_R_MALLOC_FAILURE);
            goto err;
        }
        ret->data = new_data;
    }
    ret->length = BN_bn2bin(bn, ret->data);
    /* Correct zero case */
    if (!ret->length) {
        ret->data[0] = 0;
        ret->length = 1;
    }
    return (ret);
 err:
    if (ret != ai)
        M_ASN1_INTEGER_free(ret);
    return (NULL);
}