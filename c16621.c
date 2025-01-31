fmtfp(char **sbuffer,
      char **buffer,
      size_t *currlen,
      size_t *maxlen, LDOUBLE fvalue, int min, int max, int flags)
{
    int signvalue = 0;
    LDOUBLE ufvalue;
    char iconvert[20];
    char fconvert[20];
    int iplace = 0;
    int fplace = 0;
    int padlen = 0;
    int zpadlen = 0;
    long intpart;
    long fracpart;
    long max10;

    if (max < 0)
        max = 6;
    ufvalue = abs_val(fvalue);
    if (fvalue < 0)
        signvalue = '-';
    else if (flags & DP_F_PLUS)
        signvalue = '+';
    else if (flags & DP_F_SPACE)
        signvalue = ' ';

    intpart = (long)ufvalue;

    /*
     * sorry, we only support 9 digits past the decimal because of our
     * conversion method
     */
    if (max > 9)
        max = 9;

    /*
     * we "cheat" by converting the fractional part to integer by multiplying
     * by a factor of 10
     */
    max10 = roundv(pow_10(max));
    fracpart = roundv(pow_10(max) * (ufvalue - intpart));

    if (fracpart >= max10) {
        intpart++;
        fracpart -= max10;
    }

    /* convert integer part */
    do {
        iconvert[iplace++] = "0123456789"[intpart % 10];
        intpart = (intpart / 10);
    } while (intpart && (iplace < (int)sizeof(iconvert)));
    if (iplace == sizeof iconvert)
        iplace--;
    iconvert[iplace] = 0;

    /* convert fractional part */
    do {
        fconvert[fplace++] = "0123456789"[fracpart % 10];
        fracpart = (fracpart / 10);
    } while (fplace < max);
    if (fplace == sizeof fconvert)
        fplace--;
    fconvert[fplace] = 0;

    /* -1 for decimal point, another -1 if we are printing a sign */
    padlen = min - iplace - max - 1 - ((signvalue) ? 1 : 0);
    zpadlen = max - fplace;
    if (zpadlen < 0)
        zpadlen = 0;
    if (padlen < 0)
        padlen = 0;
    if (flags & DP_F_MINUS)
        padlen = -padlen;

    if ((flags & DP_F_ZERO) && (padlen > 0)) {
        if (signvalue) {
            doapr_outch(sbuffer, buffer, currlen, maxlen, signvalue);
            --padlen;
            signvalue = 0;
        }
        while (padlen > 0) {
            doapr_outch(sbuffer, buffer, currlen, maxlen, '0');
            --padlen;
        }
    }
    while (padlen > 0) {
        doapr_outch(sbuffer, buffer, currlen, maxlen, ' ');
        --padlen;
    }
    if (signvalue)
        doapr_outch(sbuffer, buffer, currlen, maxlen, signvalue);

    while (iplace > 0)
        doapr_outch(sbuffer, buffer, currlen, maxlen, iconvert[--iplace]);

    /*
     * Decimal point. This should probably use locale to find the correct
     * char to print out.
     */
    if (max > 0 || (flags & DP_F_NUM)) {
        doapr_outch(sbuffer, buffer, currlen, maxlen, '.');

        while (fplace > 0)
            doapr_outch(sbuffer, buffer, currlen, maxlen, fconvert[--fplace]);
    }
    while (zpadlen > 0) {
        doapr_outch(sbuffer, buffer, currlen, maxlen, '0');
        --zpadlen;
    }

    while (padlen < 0) {
        doapr_outch(sbuffer, buffer, currlen, maxlen, ' ');
        ++padlen;
    }
}