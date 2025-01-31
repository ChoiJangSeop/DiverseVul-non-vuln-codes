static int tight_send_framebuffer_update(VncState *vs, int x, int y,
                                         int w, int h)
{
    int max_rows;

    if (vs->client_pf.bytes_per_pixel == 4 && vs->client_pf.rmax == 0xFF &&
        vs->client_pf.bmax == 0xFF && vs->client_pf.gmax == 0xFF) {
        vs->tight.pixel24 = true;
    } else {
        vs->tight.pixel24 = false;
    }

#ifdef CONFIG_VNC_JPEG
    if (vs->tight.quality != (uint8_t)-1) {
        double freq = vnc_update_freq(vs, x, y, w, h);

        if (freq > tight_jpeg_conf[vs->tight.quality].jpeg_freq_threshold) {
            return send_rect_simple(vs, x, y, w, h, false);
        }
    }
#endif

    if (w * h < VNC_TIGHT_MIN_SPLIT_RECT_SIZE) {
        return send_rect_simple(vs, x, y, w, h, true);
    }

    /* Calculate maximum number of rows in one non-solid rectangle. */

    max_rows = tight_conf[vs->tight.compression].max_rect_size;
    max_rows /= MIN(tight_conf[vs->tight.compression].max_rect_width, w);

    return find_large_solid_color_rect(vs, x, y, w, h, max_rows);
}