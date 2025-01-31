void rng_backend_request_entropy(RngBackend *s, size_t size,
                                 EntropyReceiveFunc *receive_entropy,
                                 void *opaque)
{
    RngBackendClass *k = RNG_BACKEND_GET_CLASS(s);

    if (k->request_entropy) {
        k->request_entropy(s, size, receive_entropy, opaque);
    }
}