static void vnc_async_encoding_start(VncState *orig, VncState *local)
{
    local->vnc_encoding = orig->vnc_encoding;
    local->features = orig->features;
    local->ds = orig->ds;
    local->vd = orig->vd;
    local->lossy_rect = orig->lossy_rect;
    local->write_pixels = orig->write_pixels;
    local->clientds = orig->clientds;
    local->tight = orig->tight;
    local->zlib = orig->zlib;
    local->hextile = orig->hextile;
    local->zrle = orig->zrle;
    local->output =  queue->buffer;
    local->csock = -1; /* Don't do any network work on this thread */

    buffer_reset(&local->output);
}