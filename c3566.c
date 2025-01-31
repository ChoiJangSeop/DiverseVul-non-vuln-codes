static void rng_egd_request_entropy(RngBackend *b, size_t size,
                                    EntropyReceiveFunc *receive_entropy,
                                    void *opaque)
{
    RngEgd *s = RNG_EGD(b);
    RngRequest *req;

    req = g_malloc(sizeof(*req));

    req->offset = 0;
    req->size = size;
    req->receive_entropy = receive_entropy;
    req->opaque = opaque;
    req->data = g_malloc(req->size);

    while (size > 0) {
        uint8_t header[2];
        uint8_t len = MIN(size, 255);

        /* synchronous entropy request */
        header[0] = 0x02;
        header[1] = len;

        qemu_chr_fe_write(s->chr, header, sizeof(header));

        size -= len;
    }

    s->parent.requests = g_slist_append(s->parent.requests, req);
}