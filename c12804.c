static void gf_isom_write_tx3g(GF_Tx3gSampleEntryBox *a, GF_BitStream *bs, u32 sidx, u32 sidx_offset)
{
	u32 size, j, fount_count;
	Bool is_qt_text = (a->type==GF_ISOM_BOX_TYPE_TEXT) ? GF_TRUE : GF_FALSE;
	const char *qt_fontname = NULL;
	void gpp_write_rgba(GF_BitStream *bs, u32 col);
	void gpp_write_box(GF_BitStream *bs, GF_BoxRecord *rec);
	void gpp_write_style(GF_BitStream *bs, GF_StyleRecord *rec);


	if (sidx_offset) gf_bs_write_u8(bs, sidx + sidx_offset);

	/*SINCE WINCE HAS A READONLY VERSION OF MP4 WE MUST DO IT BY HAND*/
	size = 8 + 18 + 8 + 12;
	size += 8 + 2;
	fount_count = 0;
	if (is_qt_text) {
		GF_TextSampleEntryBox *qt = (GF_TextSampleEntryBox *)a;
		if (qt->textName) {
			qt_fontname = qt->textName;
			fount_count = 1;
		}
	} else {
		if (a->font_table) {
			fount_count = a->font_table->entry_count;
			for (j=0; j<fount_count; j++) {
				size += 3;
				if (a->font_table->fonts[j].fontName) size += (u32) strlen(a->font_table->fonts[j].fontName);
			}
		}
	}
	/*write TextSampleEntry box*/
	gf_bs_write_u32(bs, size);
	gf_bs_write_u32(bs, a->type);
	gf_bs_write_data(bs, a->reserved, 6);
	gf_bs_write_u16(bs, a->dataReferenceIndex);
	gf_bs_write_u32(bs, a->displayFlags);
	gf_bs_write_u8(bs, a->horizontal_justification);
	gf_bs_write_u8(bs, a->vertical_justification);
	gpp_write_rgba(bs, a->back_color);
	gpp_write_box(bs, &a->default_box);
	gpp_write_style(bs, &a->default_style);
	/*write font table box*/
	size -= (8 + 18 + 8 + 12);
	gf_bs_write_u32(bs, size);
	gf_bs_write_u32(bs, GF_ISOM_BOX_TYPE_FTAB);

	gf_bs_write_u16(bs, fount_count);
	for (j=0; j<fount_count; j++) {
		if (is_qt_text) {
			gf_bs_write_u16(bs, 0);
			if (qt_fontname) {
				u32 len = (u32) strlen(qt_fontname);
				gf_bs_write_u8(bs, len);
				gf_bs_write_data(bs, qt_fontname, len);
			} else {
				gf_bs_write_u8(bs, 0);
			}
		} else {
			gf_bs_write_u16(bs, a->font_table->fonts[j].fontID);
			if (a->font_table->fonts[j].fontName) {
				u32 len = (u32) strlen(a->font_table->fonts[j].fontName);
				gf_bs_write_u8(bs, len);
				gf_bs_write_data(bs, a->font_table->fonts[j].fontName, len);
			} else {
				gf_bs_write_u8(bs, 0);
			}
		}
	}
}