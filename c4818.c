static int vnc_update_client(VncState *vs, int has_dirty)
{
    if (vs->need_update && vs->csock != -1) {
        VncDisplay *vd = vs->vd;
        VncJob *job;
        int y;
        int width, height;
        int n = 0;


        if (vs->output.offset && !vs->audio_cap && !vs->force_update)
            /* kernel send buffers are full -> drop frames to throttle */
            return 0;

        if (!has_dirty && !vs->audio_cap && !vs->force_update)
            return 0;

        /*
         * Send screen updates to the vnc client using the server
         * surface and server dirty map.  guest surface updates
         * happening in parallel don't disturb us, the next pass will
         * send them to the client.
         */
        job = vnc_job_new(vs);

        width = MIN(vd->server->width, vs->client_width);
        height = MIN(vd->server->height, vs->client_height);

        for (y = 0; y < height; y++) {
            int x;
            int last_x = -1;
            for (x = 0; x < width / 16; x++) {
                if (test_and_clear_bit(x, vs->dirty[y])) {
                    if (last_x == -1) {
                        last_x = x;
                    }
                } else {
                    if (last_x != -1) {
                        int h = find_and_clear_dirty_height(vs, y, last_x, x,
                                                            height);

                        n += vnc_job_add_rect(job, last_x * 16, y,
                                              (x - last_x) * 16, h);
                    }
                    last_x = -1;
                }
            }
            if (last_x != -1) {
                int h = find_and_clear_dirty_height(vs, y, last_x, x, height);
                n += vnc_job_add_rect(job, last_x * 16, y,
                                      (x - last_x) * 16, h);
            }
        }

        vnc_job_push(job);
        vs->force_update = 0;
        return n;
    }

    if (vs->csock == -1)
        vnc_disconnect_finish(vs);

    return 0;
}