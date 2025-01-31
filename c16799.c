LogLuvClose(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;

	/*
	 * For consistency, we always want to write out the same
	 * bitspersample and sampleformat for our TIFF file,
	 * regardless of the data format being used by the application.
	 * Since this routine is called after tags have been set but
	 * before they have been recorded in the file, we reset them here.
	 */
	td->td_samplesperpixel =
	    (td->td_photometric == PHOTOMETRIC_LOGL) ? 1 : 3;
	td->td_bitspersample = 16;
	td->td_sampleformat = SAMPLEFORMAT_INT;
}