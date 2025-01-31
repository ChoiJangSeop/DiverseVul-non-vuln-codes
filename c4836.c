static void rgb_prepare_row(VncState *vs, uint8_t *dst, int x, int y,
                            int count)
{
    if (ds_get_bytes_per_pixel(vs->ds) == 4) {
        if (vs->ds->surface->pf.rmax == 0xFF &&
            vs->ds->surface->pf.gmax == 0xFF &&
            vs->ds->surface->pf.bmax == 0xFF) {
            rgb_prepare_row24(vs, dst, x, y, count);
        } else {
            rgb_prepare_row32(vs, dst, x, y, count);
        }
    } else {
        rgb_prepare_row16(vs, dst, x, y, count);
    }
}