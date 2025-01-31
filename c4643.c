static void rlvl_destroy(jpc_enc_rlvl_t *rlvl)
{
	jpc_enc_band_t *band;
	uint_fast16_t bandno;

	if (rlvl->bands) {
		for (bandno = 0, band = rlvl->bands; bandno < rlvl->numbands;
		  ++bandno, ++band) {
			band_destroy(band);
		}
		jas_free(rlvl->bands);
	}
}