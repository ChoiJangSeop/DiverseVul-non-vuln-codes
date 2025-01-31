static int MOD_EXP_CTIME_COPY_TO_PREBUF(const BIGNUM *b, int top,
                                        unsigned char *buf, int idx,
                                        int width)
{
    size_t i, j;

    if (top > b->top)
        top = b->top;           /* this works because 'buf' is explicitly
                                 * zeroed */
    for (i = 0, j = idx; i < top * sizeof b->d[0]; i++, j += width) {
        buf[j] = ((unsigned char *)b->d)[i];
    }

    return 1;
}