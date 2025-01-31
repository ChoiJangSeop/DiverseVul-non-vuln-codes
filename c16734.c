horAcc32(TIFF* tif, uint8* cp0, tmsize_t cc)
{
	tmsize_t stride = PredictorState(tif)->stride;
	uint32* wp = (uint32*) cp0;
	tmsize_t wc = cc / 4;

	assert((cc%(4*stride))==0);

	if (wc > stride) {
		wc -= stride;
		do {
			REPEAT4(stride, wp[stride] += wp[0]; wp++)
			wc -= stride;
		} while (wc > 0);
	}
}