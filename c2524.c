void vncws_handshake_read(void *opaque)
{
    VncState *vs = opaque;
    uint8_t *handshake_end;
    long ret;
    buffer_reserve(&vs->ws_input, 4096);
    ret = vnc_client_read_buf(vs, buffer_end(&vs->ws_input), 4096);

    if (!ret) {
        if (vs->csock == -1) {
            vnc_disconnect_finish(vs);
        }
        return;
    }
    vs->ws_input.offset += ret;

    handshake_end = (uint8_t *)g_strstr_len((char *)vs->ws_input.buffer,
            vs->ws_input.offset, WS_HANDSHAKE_END);
    if (handshake_end) {
        qemu_set_fd_handler2(vs->csock, NULL, vnc_client_read, NULL, vs);
        vncws_process_handshake(vs, vs->ws_input.buffer, vs->ws_input.offset);
        buffer_advance(&vs->ws_input, handshake_end - vs->ws_input.buffer +
                strlen(WS_HANDSHAKE_END));
    }
}