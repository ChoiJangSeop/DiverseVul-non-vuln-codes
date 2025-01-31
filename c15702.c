static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    GradFunContext *s = inlink->dst->priv;
    AVFilterLink *outlink = inlink->dst->outputs[0];
    AVFrame *out;
    int p, direct;

    if (av_frame_is_writable(in)) {
        direct = 1;
        out = in;
    } else {
        direct = 0;
        out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
        if (!out) {
            av_frame_free(&in);
            return AVERROR(ENOMEM);
        }
        av_frame_copy_props(out, in);
    }

    for (p = 0; p < 4 && in->data[p]; p++) {
        int w = inlink->w;
        int h = inlink->h;
        int r = s->radius;
        if (p) {
            w = s->chroma_w;
            h = s->chroma_h;
            r = s->chroma_r;
        }

        if (FFMIN(w, h) > 2 * r)
            filter(s, out->data[p], in->data[p], w, h, out->linesize[p], in->linesize[p], r);
        else if (out->data[p] != in->data[p])
            av_image_copy_plane(out->data[p], out->linesize[p], in->data[p], in->linesize[p], w, h);
    }

    if (!direct)
        av_frame_free(&in);

    return ff_filter_frame(outlink, out);
}