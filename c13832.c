int av_reallocp_array(void *ptr, size_t nmemb, size_t size)
{
    void **ptrptr = ptr;
    *ptrptr = av_realloc_f(*ptrptr, nmemb, size);
    if (!*ptrptr && !(nmemb && size))
        return AVERROR(ENOMEM);
    return 0;
}