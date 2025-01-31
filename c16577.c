TIFFRGBAImageBegin(TIFFRGBAImage* img, TIFF* tif, int stop, char emsg[1024])
{
	uint16* sampleinfo;
	uint16 extrasamples;
	uint16 planarconfig;
	uint16 compress;
	int colorchannels;
	uint16 *red_orig, *green_orig, *blue_orig;
	int n_color;

	/* Initialize to normal values */
	img->row_offset = 0;
	img->col_offset = 0;
	img->redcmap = NULL;
	img->greencmap = NULL;
	img->bluecmap = NULL;
	img->req_orientation = ORIENTATION_BOTLEFT;     /* It is the default */

	img->tif = tif;
	img->stoponerr = stop;
	TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &img->bitspersample);
	switch (img->bitspersample) {
		case 1:
		case 2:
		case 4:
		case 8:
		case 16:
			break;
		default:
			sprintf(emsg, "Sorry, can not handle images with %d-bit samples",
			    img->bitspersample);
			goto fail_return;
	}
	img->alpha = 0;
	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &img->samplesperpixel);
	TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES,
	    &extrasamples, &sampleinfo);
	if (extrasamples >= 1)
	{
		switch (sampleinfo[0]) {
			case EXTRASAMPLE_UNSPECIFIED:          /* Workaround for some images without */
				if (img->samplesperpixel > 3)  /* correct info about alpha channel */
					img->alpha = EXTRASAMPLE_ASSOCALPHA;
				break;
			case EXTRASAMPLE_ASSOCALPHA:           /* data is pre-multiplied */
			case EXTRASAMPLE_UNASSALPHA:           /* data is not pre-multiplied */
				img->alpha = sampleinfo[0];
				break;
		}
	}

#ifdef DEFAULT_EXTRASAMPLE_AS_ALPHA
	if( !TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &img->photometric))
		img->photometric = PHOTOMETRIC_MINISWHITE;

	if( extrasamples == 0
	    && img->samplesperpixel == 4
	    && img->photometric == PHOTOMETRIC_RGB )
	{
		img->alpha = EXTRASAMPLE_ASSOCALPHA;
		extrasamples = 1;
	}
#endif

	colorchannels = img->samplesperpixel - extrasamples;
	TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &compress);
	TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planarconfig);
	if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &img->photometric)) {
		switch (colorchannels) {
			case 1:
				if (isCCITTCompression(tif))
					img->photometric = PHOTOMETRIC_MINISWHITE;
				else
					img->photometric = PHOTOMETRIC_MINISBLACK;
				break;
			case 3:
				img->photometric = PHOTOMETRIC_RGB;
				break;
			default:
				sprintf(emsg, "Missing needed %s tag", photoTag);
                                goto fail_return;
		}
	}
	switch (img->photometric) {
		case PHOTOMETRIC_PALETTE:
			if (!TIFFGetField(tif, TIFFTAG_COLORMAP,
			    &red_orig, &green_orig, &blue_orig)) {
				sprintf(emsg, "Missing required \"Colormap\" tag");
                                goto fail_return;
			}

			/* copy the colormaps so we can modify them */
			n_color = (1U << img->bitspersample);
			img->redcmap = (uint16 *) _TIFFmalloc(sizeof(uint16)*n_color);
			img->greencmap = (uint16 *) _TIFFmalloc(sizeof(uint16)*n_color);
			img->bluecmap = (uint16 *) _TIFFmalloc(sizeof(uint16)*n_color);
			if( !img->redcmap || !img->greencmap || !img->bluecmap ) {
				sprintf(emsg, "Out of memory for colormap copy");
                                goto fail_return;
			}

			_TIFFmemcpy( img->redcmap, red_orig, n_color * 2 );
			_TIFFmemcpy( img->greencmap, green_orig, n_color * 2 );
			_TIFFmemcpy( img->bluecmap, blue_orig, n_color * 2 );

			/* fall thru... */
		case PHOTOMETRIC_MINISWHITE:
		case PHOTOMETRIC_MINISBLACK:
			if (planarconfig == PLANARCONFIG_CONTIG
			    && img->samplesperpixel != 1
			    && img->bitspersample < 8 ) {
				sprintf(emsg,
				    "Sorry, can not handle contiguous data with %s=%d, "
				    "and %s=%d and Bits/Sample=%d",
				    photoTag, img->photometric,
				    "Samples/pixel", img->samplesperpixel,
				    img->bitspersample);
                                goto fail_return;
			}
			break;
		case PHOTOMETRIC_YCBCR:
			/* It would probably be nice to have a reality check here. */
			if (planarconfig == PLANARCONFIG_CONTIG)
				/* can rely on libjpeg to convert to RGB */
				/* XXX should restore current state on exit */
				switch (compress) {
					case COMPRESSION_JPEG:
						/*
						 * TODO: when complete tests verify complete desubsampling
						 * and YCbCr handling, remove use of TIFFTAG_JPEGCOLORMODE in
						 * favor of tif_getimage.c native handling
						 */
						TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
						img->photometric = PHOTOMETRIC_RGB;
						break;
					default:
						/* do nothing */;
						break;
				}
			/*
			 * TODO: if at all meaningful and useful, make more complete
			 * support check here, or better still, refactor to let supporting
			 * code decide whether there is support and what meaningfull
			 * error to return
			 */
			break;
		case PHOTOMETRIC_RGB:
			if (colorchannels < 3) {
				sprintf(emsg, "Sorry, can not handle RGB image with %s=%d",
				    "Color channels", colorchannels);
                                goto fail_return;
			}
			break;
		case PHOTOMETRIC_SEPARATED:
			{
				uint16 inkset;
				TIFFGetFieldDefaulted(tif, TIFFTAG_INKSET, &inkset);
				if (inkset != INKSET_CMYK) {
					sprintf(emsg, "Sorry, can not handle separated image with %s=%d",
					    "InkSet", inkset);
                                        goto fail_return;
				}
				if (img->samplesperpixel < 4) {
					sprintf(emsg, "Sorry, can not handle separated image with %s=%d",
					    "Samples/pixel", img->samplesperpixel);
                                        goto fail_return;
				}
			}
			break;
		case PHOTOMETRIC_LOGL:
			if (compress != COMPRESSION_SGILOG) {
				sprintf(emsg, "Sorry, LogL data must have %s=%d",
				    "Compression", COMPRESSION_SGILOG);
                                goto fail_return;
			}
			TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
			img->photometric = PHOTOMETRIC_MINISBLACK;	/* little white lie */
			img->bitspersample = 8;
			break;
		case PHOTOMETRIC_LOGLUV:
			if (compress != COMPRESSION_SGILOG && compress != COMPRESSION_SGILOG24) {
				sprintf(emsg, "Sorry, LogLuv data must have %s=%d or %d",
				    "Compression", COMPRESSION_SGILOG, COMPRESSION_SGILOG24);
                                goto fail_return;
			}
			if (planarconfig != PLANARCONFIG_CONTIG) {
				sprintf(emsg, "Sorry, can not handle LogLuv images with %s=%d",
				    "Planarconfiguration", planarconfig);
				return (0);
			}
			TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
			img->photometric = PHOTOMETRIC_RGB;		/* little white lie */
			img->bitspersample = 8;
			break;
		case PHOTOMETRIC_CIELAB:
			break;
		default:
			sprintf(emsg, "Sorry, can not handle image with %s=%d",
			    photoTag, img->photometric);
                        goto fail_return;
	}
	img->Map = NULL;
	img->BWmap = NULL;
	img->PALmap = NULL;
	img->ycbcr = NULL;
	img->cielab = NULL;
	img->UaToAa = NULL;
	img->Bitdepth16To8 = NULL;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &img->width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &img->height);
	TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &img->orientation);
	img->isContig =
	    !(planarconfig == PLANARCONFIG_SEPARATE && img->samplesperpixel > 1);
	if (img->isContig) {
		if (!PickContigCase(img)) {
			sprintf(emsg, "Sorry, can not handle image");
			goto fail_return;
		}
	} else {
		if (!PickSeparateCase(img)) {
			sprintf(emsg, "Sorry, can not handle image");
			goto fail_return;
		}
	}
	return 1;

  fail_return:
        _TIFFfree( img->redcmap );
        _TIFFfree( img->greencmap );
        _TIFFfree( img->bluecmap );
        img->redcmap = img->greencmap = img->bluecmap = NULL;
        return 0;
}