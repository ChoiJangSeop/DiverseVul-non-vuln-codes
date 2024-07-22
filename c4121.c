static int exif_process_IFD_in_MAKERNOTE(image_info_type *ImageInfo, char * value_ptr, int value_len, char *offset_base, size_t IFDlength, size_t displacement TSRMLS_DC)
{
	int de, i=0, section_index = SECTION_MAKERNOTE;
	int NumDirEntries, old_motorola_intel, offset_diff;
	const maker_note_type *maker_note;
	char *dir_start;

	for (i=0; i<=sizeof(maker_note_array)/sizeof(maker_note_type); i++) {
		if (i==sizeof(maker_note_array)/sizeof(maker_note_type))
			return FALSE;
		maker_note = maker_note_array+i;

		/*exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_NOTICE, "check (%s,%s)", maker_note->make?maker_note->make:"", maker_note->model?maker_note->model:"");*/
		if (maker_note->make && (!ImageInfo->make || strcmp(maker_note->make, ImageInfo->make)))
			continue;
		if (maker_note->model && (!ImageInfo->model || strcmp(maker_note->model, ImageInfo->model)))
			continue;
		if (maker_note->id_string && strncmp(maker_note->id_string, value_ptr, maker_note->id_string_len))
			continue;
		break;
	}

	if (maker_note->offset >= value_len) {
		/* Do not go past the value end */
		exif_error_docref("exif_read_data#error_ifd" EXIFERR_CC, ImageInfo, E_WARNING, "IFD data too short: 0x%04X offset 0x%04X", value_len, maker_note->offset);
		return FALSE;
	}

	dir_start = value_ptr + maker_note->offset;

#ifdef EXIF_DEBUG
	exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_NOTICE, "Process %s @x%04X + 0x%04X=%d: %s", exif_get_sectionname(section_index), (int)dir_start-(int)offset_base+maker_note->offset+displacement, value_len, value_len, exif_char_dump(value_ptr, value_len, (int)dir_start-(int)offset_base+maker_note->offset+displacement));
#endif

	ImageInfo->sections_found |= FOUND_MAKERNOTE;

	old_motorola_intel = ImageInfo->motorola_intel;
	switch (maker_note->byte_order) {
		case MN_ORDER_INTEL:
			ImageInfo->motorola_intel = 0;
			break;
		case MN_ORDER_MOTOROLA:
			ImageInfo->motorola_intel = 1;
			break;
		default:
		case MN_ORDER_NORMAL:
			break;
	}

	NumDirEntries = php_ifd_get16u(dir_start, ImageInfo->motorola_intel);

	switch (maker_note->offset_mode) {
		case MN_OFFSET_MAKER:
			offset_base = value_ptr;
			break;
		case MN_OFFSET_GUESS:
			if (maker_note->offset + 10 + 4 >= value_len) {
				/* Can not read dir_start+10 since it's beyond value end */
				exif_error_docref("exif_read_data#error_ifd" EXIFERR_CC, ImageInfo, E_WARNING, "IFD data too short: 0x%04X", value_len);
				return FALSE;
			}
			offset_diff = 2 + NumDirEntries*12 + 4 - php_ifd_get32u(dir_start+10, ImageInfo->motorola_intel);
#ifdef EXIF_DEBUG
			exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_NOTICE, "Using automatic offset correction: 0x%04X", ((int)dir_start-(int)offset_base+maker_note->offset+displacement) + offset_diff);
#endif
			if (offset_diff < 0 || offset_diff >= value_len ) {
				exif_error_docref("exif_read_data#error_ifd" EXIFERR_CC, ImageInfo, E_WARNING, "IFD data bad offset: 0x%04X length 0x%04X", offset_diff, value_len);
				return FALSE;
			}
			offset_base = value_ptr + offset_diff;
			break;
		default:
		case MN_OFFSET_NORMAL:
			break;
	}

	if ((2+NumDirEntries*12) > value_len) {
		exif_error_docref("exif_read_data#error_ifd" EXIFERR_CC, ImageInfo, E_WARNING, "Illegal IFD size: 2 + 0x%04X*12 = 0x%04X > 0x%04X", NumDirEntries, 2+NumDirEntries*12, value_len);
		return FALSE;
	}

	for (de=0;de<NumDirEntries;de++) {
		if (!exif_process_IFD_TAG(ImageInfo, dir_start + 2 + 12 * de,
								  offset_base, IFDlength, displacement, section_index, 0, maker_note->tag_table TSRMLS_CC)) {
			return FALSE;
		}
	}
	ImageInfo->motorola_intel = old_motorola_intel;
/*	NextDirOffset (must be NULL) = php_ifd_get32u(dir_start+2+12*de, ImageInfo->motorola_intel);*/
#ifdef EXIF_DEBUG
	exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_NOTICE, "Subsection %s done", exif_get_sectionname(SECTION_MAKERNOTE));
#endif
	return TRUE;
}