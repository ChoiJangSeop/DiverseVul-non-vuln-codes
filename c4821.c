static void vnc_dpy_resize(DisplayState *ds)
{
    VncDisplay *vd = ds->opaque;
    VncState *vs;

    vnc_abort_display_jobs(vd);

    /* server surface */
    if (!vd->server)
        vd->server = g_malloc0(sizeof(*vd->server));
    if (vd->server->data)
        g_free(vd->server->data);
    *(vd->server) = *(ds->surface);
    vd->server->data = g_malloc0(vd->server->linesize *
                                    vd->server->height);

    /* guest surface */
    if (!vd->guest.ds)
        vd->guest.ds = g_malloc0(sizeof(*vd->guest.ds));
    if (ds_get_bytes_per_pixel(ds) != vd->guest.ds->pf.bytes_per_pixel)
        console_color_init(ds);
    *(vd->guest.ds) = *(ds->surface);
    memset(vd->guest.dirty, 0xFF, sizeof(vd->guest.dirty));

    QTAILQ_FOREACH(vs, &vd->clients, next) {
        vnc_colordepth(vs);
        vnc_desktop_resize(vs);
        if (vs->vd->cursor) {
            vnc_cursor_define(vs);
        }
        memset(vs->dirty, 0xFF, sizeof(vs->dirty));
    }
}