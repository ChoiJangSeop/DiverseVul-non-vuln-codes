swabHorAcc16(TIFF* tif, uint8* cp0, tmsize_t cc)
{
	uint16* wp = (uint16*) cp0;
	tmsize_t wc = cc / 2;

        TIFFSwabArrayOfShort(wp, wc);
        horAcc16(tif, cp0, cc);
}