void vnc_convert_pixel(VncState *vs, uint8_t *buf, uint32_t v)
{
    uint8_t r, g, b;
    VncDisplay *vd = vs->vd;

    r = ((((v & vd->server->pf.rmask) >> vd->server->pf.rshift) << vs->clientds.pf.rbits) >>
        vd->server->pf.rbits);
    g = ((((v & vd->server->pf.gmask) >> vd->server->pf.gshift) << vs->clientds.pf.gbits) >>
        vd->server->pf.gbits);
    b = ((((v & vd->server->pf.bmask) >> vd->server->pf.bshift) << vs->clientds.pf.bbits) >>
        vd->server->pf.bbits);
    v = (r << vs->clientds.pf.rshift) |
        (g << vs->clientds.pf.gshift) |
        (b << vs->clientds.pf.bshift);
    switch(vs->clientds.pf.bytes_per_pixel) {
    case 1:
        buf[0] = v;
        break;
    case 2:
        if (vs->clientds.flags & QEMU_BIG_ENDIAN_FLAG) {
            buf[0] = v >> 8;
            buf[1] = v;
        } else {
            buf[1] = v >> 8;
            buf[0] = v;
        }
        break;
    default:
    case 4:
        if (vs->clientds.flags & QEMU_BIG_ENDIAN_FLAG) {
            buf[0] = v >> 24;
            buf[1] = v >> 16;
            buf[2] = v >> 8;
            buf[3] = v;
        } else {
            buf[3] = v >> 24;
            buf[2] = v >> 16;
            buf[1] = v >> 8;
            buf[0] = v;
        }
        break;
    }
}