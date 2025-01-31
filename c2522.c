long vnc_client_read_ws(VncState *vs)
{
    int ret, err;
    uint8_t *payload;
    size_t payload_size, frame_size;
    VNC_DEBUG("Read websocket %p size %zd offset %zd\n", vs->ws_input.buffer,
            vs->ws_input.capacity, vs->ws_input.offset);
    buffer_reserve(&vs->ws_input, 4096);
    ret = vnc_client_read_buf(vs, buffer_end(&vs->ws_input), 4096);
    if (!ret) {
        return 0;
    }
    vs->ws_input.offset += ret;

    /* make sure that nothing is left in the ws_input buffer */
    do {
        err = vncws_decode_frame(&vs->ws_input, &payload,
                              &payload_size, &frame_size);
        if (err <= 0) {
            return err;
        }

        buffer_reserve(&vs->input, payload_size);
        buffer_append(&vs->input, payload, payload_size);

        buffer_advance(&vs->ws_input, frame_size);
    } while (vs->ws_input.offset > 0);

    return ret;
}