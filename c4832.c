static bool check_solid_tile(VncState *vs, int x, int y, int w, int h,
                             uint32_t* color, bool samecolor)
{
    VncDisplay *vd = vs->vd;

    switch(vd->server->pf.bytes_per_pixel) {
    case 4:
        return check_solid_tile32(vs, x, y, w, h, color, samecolor);
    case 2:
        return check_solid_tile16(vs, x, y, w, h, color, samecolor);
    default:
        return check_solid_tile8(vs, x, y, w, h, color, samecolor);
    }
}