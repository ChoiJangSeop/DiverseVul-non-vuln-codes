static int store_icy(URLContext *h, int size)
{
    HTTPContext *s = h->priv_data;
    /* until next metadata packet */
    int remaining = s->icy_metaint - s->icy_data_read;

    if (remaining < 0)
        return AVERROR_INVALIDDATA;

    if (!remaining) {
        /* The metadata packet is variable sized. It has a 1 byte header
         * which sets the length of the packet (divided by 16). If it's 0,
         * the metadata doesn't change. After the packet, icy_metaint bytes
         * of normal data follows. */
        uint8_t ch;
        int len = http_read_stream_all(h, &ch, 1);
        if (len < 0)
            return len;
        if (ch > 0) {
            char data[255 * 16 + 1];
            int ret;
            len = ch * 16;
            ret = http_read_stream_all(h, data, len);
            if (ret < 0)
                return ret;
            data[len + 1] = 0;
            if ((ret = av_opt_set(s, "icy_metadata_packet", data, 0)) < 0)
                return ret;
            update_metadata(s, data);
        }
        s->icy_data_read = 0;
        remaining        = s->icy_metaint;
    }

    return FFMIN(size, remaining);
}