static int mpeg4_decode_profile_level(MpegEncContext *s, GetBitContext *gb)
{

    s->avctx->profile = get_bits(gb, 4);
    s->avctx->level   = get_bits(gb, 4);

    // for Simple profile, level 0
    if (s->avctx->profile == 0 && s->avctx->level == 8) {
        s->avctx->level = 0;
    }

    return 0;
}