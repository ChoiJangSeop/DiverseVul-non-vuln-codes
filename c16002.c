static int http_connect(URLContext *h, const char *path, const char *local_path,
                        const char *hoststr, const char *auth,
                        const char *proxyauth, int *new_location)
{
    HTTPContext *s = h->priv_data;
    int post, err;
    char headers[HTTP_HEADERS_SIZE] = "";
    char *authstr = NULL, *proxyauthstr = NULL;
    int64_t off = s->off;
    int len = 0;
    const char *method;
    int send_expect_100 = 0;

    /* send http header */
    post = h->flags & AVIO_FLAG_WRITE;

    if (s->post_data) {
        /* force POST method and disable chunked encoding when
         * custom HTTP post data is set */
        post            = 1;
        s->chunked_post = 0;
    }

    if (s->method)
        method = s->method;
    else
        method = post ? "POST" : "GET";

    authstr      = ff_http_auth_create_response(&s->auth_state, auth,
                                                local_path, method);
    proxyauthstr = ff_http_auth_create_response(&s->proxy_auth_state, proxyauth,
                                                local_path, method);
    if (post && !s->post_data) {
        send_expect_100 = s->send_expect_100;
        /* The user has supplied authentication but we don't know the auth type,
         * send Expect: 100-continue to get the 401 response including the
         * WWW-Authenticate header, or an 100 continue if no auth actually
         * is needed. */
        if (auth && *auth &&
            s->auth_state.auth_type == HTTP_AUTH_NONE &&
            s->http_code != 401)
            send_expect_100 = 1;
    }

#if FF_API_HTTP_USER_AGENT
    if (strcmp(s->user_agent_deprecated, DEFAULT_USER_AGENT)) {
        av_log(s, AV_LOG_WARNING, "the user-agent option is deprecated, please use user_agent option\n");
        s->user_agent = av_strdup(s->user_agent_deprecated);
    }
#endif
    /* set default headers if needed */
    if (!has_header(s->headers, "\r\nUser-Agent: "))
        len += av_strlcatf(headers + len, sizeof(headers) - len,
                           "User-Agent: %s\r\n", s->user_agent);
    if (!has_header(s->headers, "\r\nAccept: "))
        len += av_strlcpy(headers + len, "Accept: */*\r\n",
                          sizeof(headers) - len);
    // Note: we send this on purpose even when s->off is 0 when we're probing,
    // since it allows us to detect more reliably if a (non-conforming)
    // server supports seeking by analysing the reply headers.
    if (!has_header(s->headers, "\r\nRange: ") && !post && (s->off > 0 || s->end_off || s->seekable == -1)) {
        len += av_strlcatf(headers + len, sizeof(headers) - len,
                           "Range: bytes=%"PRId64"-", s->off);
        if (s->end_off)
            len += av_strlcatf(headers + len, sizeof(headers) - len,
                               "%"PRId64, s->end_off - 1);
        len += av_strlcpy(headers + len, "\r\n",
                          sizeof(headers) - len);
    }
    if (send_expect_100 && !has_header(s->headers, "\r\nExpect: "))
        len += av_strlcatf(headers + len, sizeof(headers) - len,
                           "Expect: 100-continue\r\n");

    if (!has_header(s->headers, "\r\nConnection: ")) {
        if (s->multiple_requests)
            len += av_strlcpy(headers + len, "Connection: keep-alive\r\n",
                              sizeof(headers) - len);
        else
            len += av_strlcpy(headers + len, "Connection: close\r\n",
                              sizeof(headers) - len);
    }

    if (!has_header(s->headers, "\r\nHost: "))
        len += av_strlcatf(headers + len, sizeof(headers) - len,
                           "Host: %s\r\n", hoststr);
    if (!has_header(s->headers, "\r\nContent-Length: ") && s->post_data)
        len += av_strlcatf(headers + len, sizeof(headers) - len,
                           "Content-Length: %d\r\n", s->post_datalen);

    if (!has_header(s->headers, "\r\nContent-Type: ") && s->content_type)
        len += av_strlcatf(headers + len, sizeof(headers) - len,
                           "Content-Type: %s\r\n", s->content_type);
    if (!has_header(s->headers, "\r\nCookie: ") && s->cookies) {
        char *cookies = NULL;
        if (!get_cookies(s, &cookies, path, hoststr) && cookies) {
            len += av_strlcatf(headers + len, sizeof(headers) - len,
                               "Cookie: %s\r\n", cookies);
            av_free(cookies);
        }
    }
    if (!has_header(s->headers, "\r\nIcy-MetaData: ") && s->icy)
        len += av_strlcatf(headers + len, sizeof(headers) - len,
                           "Icy-MetaData: %d\r\n", 1);

    /* now add in custom headers */
    if (s->headers)
        av_strlcpy(headers + len, s->headers, sizeof(headers) - len);

    snprintf(s->buffer, sizeof(s->buffer),
             "%s %s HTTP/1.1\r\n"
             "%s"
             "%s"
             "%s"
             "%s%s"
             "\r\n",
             method,
             path,
             post && s->chunked_post ? "Transfer-Encoding: chunked\r\n" : "",
             headers,
             authstr ? authstr : "",
             proxyauthstr ? "Proxy-" : "", proxyauthstr ? proxyauthstr : "");

    av_log(h, AV_LOG_DEBUG, "request: %s\n", s->buffer);

    if ((err = ffurl_write(s->hd, s->buffer, strlen(s->buffer))) < 0)
        goto done;

    if (s->post_data)
        if ((err = ffurl_write(s->hd, s->post_data, s->post_datalen)) < 0)
            goto done;

    /* init input buffer */
    s->buf_ptr          = s->buffer;
    s->buf_end          = s->buffer;
    s->line_count       = 0;
    s->off              = 0;
    s->icy_data_read    = 0;
    s->filesize         = -1;
    s->willclose        = 0;
    s->end_chunked_post = 0;
    s->end_header       = 0;
    if (post && !s->post_data && !send_expect_100) {
        /* Pretend that it did work. We didn't read any header yet, since
         * we've still to send the POST data, but the code calling this
         * function will check http_code after we return. */
        s->http_code = 200;
        err = 0;
        goto done;
    }

    /* wait for header */
    err = http_read_header(h, new_location);
    if (err < 0)
        goto done;

    if (*new_location)
        s->off = off;

    err = (off == s->off) ? 0 : -1;
done:
    av_freep(&authstr);
    av_freep(&proxyauthstr);
    return err;
}