EC_KEY *d2i_ECParameters(EC_KEY **a, const unsigned char **in, long len)
{
    EC_KEY *ret;

    if (in == NULL || *in == NULL) {
        ECerr(EC_F_D2I_ECPARAMETERS, ERR_R_PASSED_NULL_PARAMETER);
        return NULL;
    }

    if (a == NULL || *a == NULL) {
        if ((ret = EC_KEY_new()) == NULL) {
            ECerr(EC_F_D2I_ECPARAMETERS, ERR_R_MALLOC_FAILURE);
            return NULL;
        }
        if (a)
            *a = ret;
    } else
        ret = *a;

    if (!d2i_ECPKParameters(&ret->group, in, len)) {
        ECerr(EC_F_D2I_ECPARAMETERS, ERR_R_EC_LIB);
        return NULL;
    }

    return ret;
}