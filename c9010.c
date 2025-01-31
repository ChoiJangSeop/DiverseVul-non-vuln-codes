exif_mnote_data_fuji_load (ExifMnoteData *en,
	const unsigned char *buf, unsigned int buf_size)
{
	ExifMnoteDataFuji *n = (ExifMnoteDataFuji*) en;
	ExifLong c;
	size_t i, tcount, o, datao;

	if (!n || !buf || !buf_size) {
		exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
			  "ExifMnoteDataFuji", "Short MakerNote");
		return;
	}
	datao = 6 + n->offset;
	if (CHECKOVERFLOW(datao, buf_size, 12)) {
		exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
			  "ExifMnoteDataFuji", "Short MakerNote");
		return;
	}

	n->order = EXIF_BYTE_ORDER_INTEL;

	datao += exif_get_long (buf + datao + 8, EXIF_BYTE_ORDER_INTEL);
	if (CHECKOVERFLOW(datao, buf_size, 2)) {
		exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
			  "ExifMnoteDataFuji", "Short MakerNote");
		return;
	}

	/* Read the number of tags */
	c = exif_get_short (buf + datao, EXIF_BYTE_ORDER_INTEL);
	datao += 2;

	/* Remove any old entries */
	exif_mnote_data_fuji_clear (n);

	/* Reserve enough space for all the possible MakerNote tags */
	n->entries = exif_mem_alloc (en->mem, sizeof (MnoteFujiEntry) * c);
	if (!n->entries) {
		EXIF_LOG_NO_MEMORY(en->log, "ExifMnoteDataFuji", sizeof (MnoteFujiEntry) * c);
		return;
	}

	/* Parse all c entries, storing ones that are successfully parsed */
	tcount = 0;
	for (i = c, o = datao; i; --i, o += 12) {
		size_t s;

		if (CHECKOVERFLOW(o, buf_size, 12)) {
			exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
				  "ExifMnoteDataFuji", "Short MakerNote");
			break;
		}

		n->entries[tcount].tag        = exif_get_short (buf + o, n->order);
		n->entries[tcount].format     = exif_get_short (buf + o + 2, n->order);
		n->entries[tcount].components = exif_get_long (buf + o + 4, n->order);
		n->entries[tcount].order      = n->order;

		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataFuji",
			  "Loading entry 0x%x ('%s')...", n->entries[tcount].tag,
			  mnote_fuji_tag_get_name (n->entries[tcount].tag));

		/* Check if we overflow the multiplication. Use buf_size as the max size for integer overflow detection,
		 * we will check the buffer sizes closer later. */
		if (	exif_format_get_size (n->entries[tcount].format) &&
			buf_size / exif_format_get_size (n->entries[tcount].format) < n->entries[tcount].components
		) {
			exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
					  "ExifMnoteDataFuji", "Tag size overflow detected (%u * %lu)", exif_format_get_size (n->entries[tcount].format), n->entries[tcount].components);
			continue;
		}
		/*
		 * Size? If bigger than 4 bytes, the actual data is not
		 * in the entry but somewhere else (offset).
		 */
		s = exif_format_get_size (n->entries[tcount].format) * n->entries[tcount].components;
		n->entries[tcount].size = s;
		if (s) {
			size_t dataofs = o + 8;
			if (s > 4)
				/* The data in this case is merely a pointer */
				dataofs = exif_get_long (buf + dataofs, n->order) + 6 + n->offset;

			if (CHECKOVERFLOW(dataofs, buf_size, s)) {
				exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
						  "ExifMnoteDataFuji", "Tag data past end of "
					  "buffer (%u >= %u)", (unsigned)(dataofs + s), buf_size);
				continue;
			}

			n->entries[tcount].data = exif_mem_alloc (en->mem, s);
			if (!n->entries[tcount].data) {
				EXIF_LOG_NO_MEMORY(en->log, "ExifMnoteDataFuji", s);
				continue;
			}
			memcpy (n->entries[tcount].data, buf + dataofs, s);
		}

		/* Tag was successfully parsed */
		++tcount;
	}
	/* Store the count of successfully parsed tags */
	n->count = tcount;
}