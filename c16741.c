swabHorDiff32(TIFF* tif, uint8* cp0, tmsize_t cc)
{
    uint32* wp = (uint32*) cp0;
    tmsize_t wc = cc / 4;

    horDiff32(tif, cp0, cc);

    TIFFSwabArrayOfLong(wp, wc);
}