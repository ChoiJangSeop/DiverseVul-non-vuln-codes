static int www_body(int s, int stype, unsigned char *context)
{
    char *buf = NULL;
    int ret = 1;
    int i, j, k, dot;
    SSL *con;
    const SSL_CIPHER *c;
    BIO *io, *ssl_bio, *sbio;
#ifdef RENEG
    int total_bytes = 0;
#endif
    int width;
    fd_set readfds;

    /* Set width for a select call if needed */
    width = s + 1;

    buf = app_malloc(bufsize, "server www buffer");
    io = BIO_new(BIO_f_buffer());
    ssl_bio = BIO_new(BIO_f_ssl());
    if ((io == NULL) || (ssl_bio == NULL))
        goto err;

#ifdef FIONBIO
    if (s_nbio) {
        unsigned long sl = 1;

        if (!s_quiet)
            BIO_printf(bio_err, "turning on non blocking io\n");
        if (BIO_socket_ioctl(s, FIONBIO, &sl) < 0)
            ERR_print_errors(bio_err);
    }
#endif

    /* lets make the output buffer a reasonable size */
    if (!BIO_set_write_buffer_size(io, bufsize))
        goto err;

    if ((con = SSL_new(ctx)) == NULL)
        goto err;

    if (s_tlsextdebug) {
        SSL_set_tlsext_debug_callback(con, tlsext_cb);
        SSL_set_tlsext_debug_arg(con, bio_s_out);
    }

    if (context && !SSL_set_session_id_context(con, context,
                        strlen((char *)context)))
        goto err;

    sbio = BIO_new_socket(s, BIO_NOCLOSE);
    if (s_nbio_test) {
        BIO *test;

        test = BIO_new(BIO_f_nbio_test());
        sbio = BIO_push(test, sbio);
    }
    SSL_set_bio(con, sbio, sbio);
    SSL_set_accept_state(con);

    /* SSL_set_fd(con,s); */
    BIO_set_ssl(ssl_bio, con, BIO_CLOSE);
    BIO_push(io, ssl_bio);
#ifdef CHARSET_EBCDIC
    io = BIO_push(BIO_new(BIO_f_ebcdic_filter()), io);
#endif

    if (s_debug) {
        BIO_set_callback(SSL_get_rbio(con), bio_dump_callback);
        BIO_set_callback_arg(SSL_get_rbio(con), (char *)bio_s_out);
    }
    if (s_msg) {
#ifndef OPENSSL_NO_SSL_TRACE
        if (s_msg == 2)
            SSL_set_msg_callback(con, SSL_trace);
        else
#endif
            SSL_set_msg_callback(con, msg_cb);
        SSL_set_msg_callback_arg(con, bio_s_msg ? bio_s_msg : bio_s_out);
    }

    for (;;) {
        i = BIO_gets(io, buf, bufsize - 1);
        if (i < 0) {            /* error */
            if (!BIO_should_retry(io) && !SSL_waiting_for_async(con)) {
                if (!s_quiet)
                    ERR_print_errors(bio_err);
                goto err;
            } else {
                BIO_printf(bio_s_out, "read R BLOCK\n");
#ifndef OPENSSL_NO_SRP
                if (BIO_should_io_special(io)
                    && BIO_get_retry_reason(io) == BIO_RR_SSL_X509_LOOKUP) {
                    BIO_printf(bio_s_out, "LOOKUP renego during read\n");
                    srp_callback_parm.user =
                        SRP_VBASE_get_by_user(srp_callback_parm.vb,
                                              srp_callback_parm.login);
                    if (srp_callback_parm.user)
                        BIO_printf(bio_s_out, "LOOKUP done %s\n",
                                   srp_callback_parm.user->info);
                    else
                        BIO_printf(bio_s_out, "LOOKUP not successful\n");
                    continue;
                }
#endif
#if defined(OPENSSL_SYS_NETWARE)
                delay(1000);
#elif !defined(OPENSSL_SYS_MSDOS)
                sleep(1);
#endif
                continue;
            }
        } else if (i == 0) {    /* end of input */
            ret = 1;
            goto end;
        }

        /* else we have data */
        if (((www == 1) && (strncmp("GET ", buf, 4) == 0)) ||
            ((www == 2) && (strncmp("GET /stats ", buf, 11) == 0))) {
            char *p;
            X509 *peer;
            STACK_OF(SSL_CIPHER) *sk;
            static const char *space = "                          ";

            if (www == 1 && strncmp("GET /reneg", buf, 10) == 0) {
                if (strncmp("GET /renegcert", buf, 14) == 0)
                    SSL_set_verify(con,
                                   SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE,
                                   NULL);
                i = SSL_renegotiate(con);
                BIO_printf(bio_s_out, "SSL_renegotiate -> %d\n", i);
                /* Send the HelloRequest */
                i = SSL_do_handshake(con);
                if (i <= 0) {
                    BIO_printf(bio_s_out, "SSL_do_handshake() Retval %d\n",
                               SSL_get_error(con, i));
                    ERR_print_errors(bio_err);
                    goto err;
                }
                /* Wait for a ClientHello to come back */
                FD_ZERO(&readfds);
                openssl_fdset(s, &readfds);
                i = select(width, (void *)&readfds, NULL, NULL, NULL);
                if (i <= 0 || !FD_ISSET(s, &readfds)) {
                    BIO_printf(bio_s_out, "Error waiting for client response\n");
                    ERR_print_errors(bio_err);
                    goto err;
                }
                /*
                 * We're not acutally expecting any data here and we ignore
                 * any that is sent. This is just to force the handshake that
                 * we're expecting to come from the client. If they haven't
                 * sent one there's not much we can do.
                 */
                BIO_gets(io, buf, bufsize - 1);
            }

            BIO_puts(io,
                     "HTTP/1.0 200 ok\r\nContent-type: text/html\r\n\r\n");
            BIO_puts(io, "<HTML><BODY BGCOLOR=\"#ffffff\">\n");
            BIO_puts(io, "<pre>\n");
/*                      BIO_puts(io,OpenSSL_version(OPENSSL_VERSION));*/
            BIO_puts(io, "\n");
            for (i = 0; i < local_argc; i++) {
                const char *myp;
                for (myp = local_argv[i]; *myp; myp++)
                    switch (*myp) {
                    case '<':
                        BIO_puts(io, "&lt;");
                        break;
                    case '>':
                        BIO_puts(io, "&gt;");
                        break;
                    case '&':
                        BIO_puts(io, "&amp;");
                        break;
                    default:
                        BIO_write(io, myp, 1);
                        break;
                    }
                BIO_write(io, " ", 1);
            }
            BIO_puts(io, "\n");

            BIO_printf(io,
                       "Secure Renegotiation IS%s supported\n",
                       SSL_get_secure_renegotiation_support(con) ?
                       "" : " NOT");

            /*
             * The following is evil and should not really be done
             */
            BIO_printf(io, "Ciphers supported in s_server binary\n");
            sk = SSL_get_ciphers(con);
            j = sk_SSL_CIPHER_num(sk);
            for (i = 0; i < j; i++) {
                c = sk_SSL_CIPHER_value(sk, i);
                BIO_printf(io, "%-11s:%-25s ",
                           SSL_CIPHER_get_version(c), SSL_CIPHER_get_name(c));
                if ((((i + 1) % 2) == 0) && (i + 1 != j))
                    BIO_puts(io, "\n");
            }
            BIO_puts(io, "\n");
            p = SSL_get_shared_ciphers(con, buf, bufsize);
            if (p != NULL) {
                BIO_printf(io,
                           "---\nCiphers common between both SSL end points:\n");
                j = i = 0;
                while (*p) {
                    if (*p == ':') {
                        BIO_write(io, space, 26 - j);
                        i++;
                        j = 0;
                        BIO_write(io, ((i % 3) ? " " : "\n"), 1);
                    } else {
                        BIO_write(io, p, 1);
                        j++;
                    }
                    p++;
                }
                BIO_puts(io, "\n");
            }
            ssl_print_sigalgs(io, con);
#ifndef OPENSSL_NO_EC
            ssl_print_curves(io, con, 0);
#endif
            BIO_printf(io, (SSL_session_reused(con)
                            ? "---\nReused, " : "---\nNew, "));
            c = SSL_get_current_cipher(con);
            BIO_printf(io, "%s, Cipher is %s\n",
                       SSL_CIPHER_get_version(c), SSL_CIPHER_get_name(c));
            SSL_SESSION_print(io, SSL_get_session(con));
            BIO_printf(io, "---\n");
            print_stats(io, SSL_get_SSL_CTX(con));
            BIO_printf(io, "---\n");
            peer = SSL_get_peer_certificate(con);
            if (peer != NULL) {
                BIO_printf(io, "Client certificate\n");
                X509_print(io, peer);
                PEM_write_bio_X509(io, peer);
            } else
                BIO_puts(io, "no client certificate available\n");
            BIO_puts(io, "</BODY></HTML>\r\n\r\n");
            break;
        } else if ((www == 2 || www == 3)
                   && (strncmp("GET /", buf, 5) == 0)) {
            BIO *file;
            char *p, *e;
            static const char *text =
                "HTTP/1.0 200 ok\r\nContent-type: text/plain\r\n\r\n";

            /* skip the '/' */
            p = &(buf[5]);

            dot = 1;
            for (e = p; *e != '\0'; e++) {
                if (e[0] == ' ')
                    break;

                switch (dot) {
                case 1:
                    dot = (e[0] == '.') ? 2 : 0;
                    break;
                case 2:
                    dot = (e[0] == '.') ? 3 : 0;
                    break;
                case 3:
                    dot = (e[0] == '/') ? -1 : 0;
                    break;
                }
                if (dot == 0)
                    dot = (e[0] == '/') ? 1 : 0;
            }
            dot = (dot == 3) || (dot == -1); /* filename contains ".."
                                              * component */

            if (*e == '\0') {
                BIO_puts(io, text);
                BIO_printf(io, "'%s' is an invalid file name\r\n", p);
                break;
            }
            *e = '\0';

            if (dot) {
                BIO_puts(io, text);
                BIO_printf(io, "'%s' contains '..' reference\r\n", p);
                break;
            }

            if (*p == '/') {
                BIO_puts(io, text);
                BIO_printf(io, "'%s' is an invalid path\r\n", p);
                break;
            }

            /* if a directory, do the index thang */
            if (app_isdir(p) > 0) {
                BIO_puts(io, text);
                BIO_printf(io, "'%s' is a directory\r\n", p);
                break;
            }

            if ((file = BIO_new_file(p, "r")) == NULL) {
                BIO_puts(io, text);
                BIO_printf(io, "Error opening '%s'\r\n", p);
                ERR_print_errors(io);
                break;
            }

            if (!s_quiet)
                BIO_printf(bio_err, "FILE:%s\n", p);

            if (www == 2) {
                i = strlen(p);
                if (((i > 5) && (strcmp(&(p[i - 5]), ".html") == 0)) ||
                    ((i > 4) && (strcmp(&(p[i - 4]), ".php") == 0)) ||
                    ((i > 4) && (strcmp(&(p[i - 4]), ".htm") == 0)))
                    BIO_puts(io,
                             "HTTP/1.0 200 ok\r\nContent-type: text/html\r\n\r\n");
                else
                    BIO_puts(io,
                             "HTTP/1.0 200 ok\r\nContent-type: text/plain\r\n\r\n");
            }
            /* send the file */
            for (;;) {
                i = BIO_read(file, buf, bufsize);
                if (i <= 0)
                    break;

#ifdef RENEG
                total_bytes += i;
                BIO_printf(bio_err, "%d\n", i);
                if (total_bytes > 3 * 1024) {
                    total_bytes = 0;
                    BIO_printf(bio_err, "RENEGOTIATE\n");
                    SSL_renegotiate(con);
                }
#endif

                for (j = 0; j < i;) {
#ifdef RENEG
                    {
                        static count = 0;
                        if (++count == 13) {
                            SSL_renegotiate(con);
                        }
                    }
#endif
                    k = BIO_write(io, &(buf[j]), i - j);
                    if (k <= 0) {
                        if (!BIO_should_retry(io)  && !SSL_waiting_for_async(con))
                            goto write_error;
                        else {
                            BIO_printf(bio_s_out, "rwrite W BLOCK\n");
                        }
                    } else {
                        j += k;
                    }
                }
            }
 write_error:
            BIO_free(file);
            break;
        }
    }

    for (;;) {
        i = (int)BIO_flush(io);
        if (i <= 0) {
            if (!BIO_should_retry(io))
                break;
        } else
            break;
    }
 end:
    /* make sure we re-use sessions */
    SSL_set_shutdown(con, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);

 err:
    if (ret >= 0)
        BIO_printf(bio_s_out, "ACCEPT\n");
    OPENSSL_free(buf);
    BIO_free_all(io);
    return (ret);
}