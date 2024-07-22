int LibRaw::unpack(void)
{
  CHECK_ORDER_HIGH(LIBRAW_PROGRESS_LOAD_RAW);
  CHECK_ORDER_LOW(LIBRAW_PROGRESS_IDENTIFY);
  try {

    if(!libraw_internal_data.internal_data.input)
      return LIBRAW_INPUT_CLOSED;

    RUN_CALLBACK(LIBRAW_PROGRESS_LOAD_RAW,0,2);
    if (O.shot_select >= P1.raw_count)
      return LIBRAW_REQUEST_FOR_NONEXISTENT_IMAGE;

    if(!load_raw)
      return LIBRAW_UNSPECIFIED_ERROR;

    // already allocated ?
    if(imgdata.image)
      {
        free(imgdata.image);
        imgdata.image = 0;
      }
    if(imgdata.rawdata.raw_alloc)
      {
        free(imgdata.rawdata.raw_alloc);
        imgdata.rawdata.raw_alloc = 0;
      }
    if (libraw_internal_data.unpacker_data.meta_length)
      {
        libraw_internal_data.internal_data.meta_data =
          (char *) malloc (libraw_internal_data.unpacker_data.meta_length);
        merror (libraw_internal_data.internal_data.meta_data, "LibRaw::unpack()");
      }

    libraw_decoder_info_t decoder_info;
    get_decoder_info(&decoder_info);

    int save_iwidth = S.iwidth, save_iheight = S.iheight, save_shrink = IO.shrink;

    int rwidth = S.raw_width, rheight = S.raw_height;
    if( !IO.fuji_width)
      {
        // adjust non-Fuji allocation
        if(rwidth < S.width + S.left_margin)
          rwidth = S.width + S.left_margin;
        if(rheight < S.height + S.top_margin)
          rheight = S.height + S.top_margin;
      }

    imgdata.rawdata.raw_image = 0;
    imgdata.rawdata.color4_image = 0;
    imgdata.rawdata.color3_image = 0;
	imgdata.rawdata.float_image = 0;
	imgdata.rawdata.float3_image = 0;

#ifdef USE_DNGSDK
	if(imgdata.idata.dng_version && dnghost && valid_for_dngsdk() && load_raw != &LibRaw::pentax_4shot_load_raw)
	{
		int rr = try_dngsdk();
	}
#endif

#ifdef USE_RAWSPEED
	if(!raw_was_read())
	{
		int rawspeed_enabled = 1;

		if(imgdata.idata.dng_version && libraw_internal_data.unpacker_data.tiff_samples == 2)
			rawspeed_enabled = 0;

		if(imgdata.idata.raw_count > 1)
			rawspeed_enabled = 0;

		// Disable rawspeed for double-sized Oly files
		if(!strncasecmp(imgdata.idata.make,"Olympus",7) &&
			( ( imgdata.sizes.raw_width > 6000) || !strncasecmp(imgdata.idata.model,"SH-2",4) || !strncasecmp(imgdata.idata.model,"SH-3",4) || !strncasecmp(imgdata.idata.model,"TG-4",4))
			)
			rawspeed_enabled = 0;

		if(imgdata.idata.dng_version && imgdata.idata.filters==0 && libraw_internal_data.unpacker_data.tiff_bps == 8) // Disable for 8 bit
			rawspeed_enabled = 0;

		if(load_raw == &LibRaw::packed_load_raw && !strncasecmp(imgdata.idata.make,"Nikon",5) && !strncasecmp(imgdata.idata.model,"E",1) )
			rawspeed_enabled = 0;

		// RawSpeed Supported,
		if(O.use_rawspeed  && rawspeed_enabled
			&& !(is_sraw() && (O.raw_processing_options & (LIBRAW_PROCESSING_SRAW_NO_RGB | LIBRAW_PROCESSING_SRAW_NO_INTERPOLATE)))
			&& (decoder_info.decoder_flags & LIBRAW_DECODER_TRYRAWSPEED) && _rawspeed_camerameta)
		{
			int rr = try_rawspeed();
		}
	}
#endif
    if(!raw_was_read()) //RawSpeed failed or not run
      {
        // Not allocated on RawSpeed call, try call LibRaow
		int zero_rawimage = 0;
        if(decoder_info.decoder_flags &  LIBRAW_DECODER_OWNALLOC)
          {
            // x3f foveon decoder and DNG float
            // Do nothing! Decoder will allocate data internally
          }
        else if(imgdata.idata.filters || P1.colors == 1) // Bayer image or single color -> decode to raw_image
          {
            imgdata.rawdata.raw_alloc = malloc(rwidth*(rheight+8)*sizeof(imgdata.rawdata.raw_image[0]));
            imgdata.rawdata.raw_image = (ushort*) imgdata.rawdata.raw_alloc;
            if(!S.raw_pitch)
                S.raw_pitch = S.raw_width*2; // Bayer case, not set before
          }
        else // NO LEGACY FLAG if (decoder_info.decoder_flags & LIBRAW_DECODER_LEGACY)
          {
            // sRAW and old Foveon decoders only, so extra buffer size is just 1/4
            S.iwidth = S.width;
            S.iheight= S.height;
            IO.shrink = 0;
			if(!S.raw_pitch)
				S.raw_pitch = (decoder_info.decoder_flags & LIBRAW_DECODER_LEGACY_WITH_MARGINS) ? S.raw_width*8 : S.width*8;
            // allocate image as temporary buffer, size
            imgdata.rawdata.raw_alloc = 0;
            imgdata.image = (ushort (*)[4]) calloc(unsigned(S.raw_width)*unsigned(S.raw_height),sizeof(*imgdata.image));
			if(!(decoder_info.decoder_flags &  LIBRAW_DECODER_ADOBECOPYPIXEL))
			{
				imgdata.rawdata.raw_image = (ushort*) imgdata.image ;
				zero_rawimage = 1;
			}
          }
        ID.input->seek(libraw_internal_data.unpacker_data.data_offset, SEEK_SET);

        unsigned m_save = C.maximum;
        if(load_raw == &LibRaw::unpacked_load_raw && !strcasecmp(imgdata.idata.make,"Nikon"))
          C.maximum=65535;
        (this->*load_raw)();
		if(zero_rawimage)
			imgdata.rawdata.raw_image = 0;
        if(load_raw == &LibRaw::unpacked_load_raw && !strcasecmp(imgdata.idata.make,"Nikon"))
          C.maximum = m_save;
        if(decoder_info.decoder_flags &  LIBRAW_DECODER_OWNALLOC)
          {
            // x3f foveon decoder only: do nothing

          }
        else if (!(imgdata.idata.filters || P1.colors == 1) ) // legacy decoder, ownalloc handled above
          {
            // successfully decoded legacy image, attach image to raw_alloc
            imgdata.rawdata.raw_alloc = imgdata.image;
		    imgdata.rawdata.color4_image = (ushort (*)[4]) imgdata.rawdata.raw_alloc;
            imgdata.image = 0;
            // Restore saved values. Note: Foveon have masked frame
            // Other 4-color legacy data: no borders
			if(!(libraw_internal_data.unpacker_data.load_flags & 256))
			{
				S.raw_width = S.width;
				S.left_margin = 0;
				S.raw_height = S.height;
				S.top_margin = 0;
			}
          }
      }

    if(imgdata.rawdata.raw_image)
      crop_masked_pixels(); // calculate black levels

    // recover image sizes
    S.iwidth = save_iwidth;
    S.iheight = save_iheight;
    IO.shrink = save_shrink;

    // adjust black to possible maximum
    unsigned int i = C.cblack[3];
    unsigned int c;
    for(c=0;c<3;c++)
      if (i > C.cblack[c]) i = C.cblack[c];
    for (c=0;c<4;c++)
      C.cblack[c] -= i;
    C.black += i;

    // Save color,sizes and internal data into raw_image fields
    memmove(&imgdata.rawdata.color,&imgdata.color,sizeof(imgdata.color));
    memmove(&imgdata.rawdata.sizes,&imgdata.sizes,sizeof(imgdata.sizes));
    memmove(&imgdata.rawdata.iparams,&imgdata.idata,sizeof(imgdata.idata));
    memmove(&imgdata.rawdata.ioparams,&libraw_internal_data.internal_output_params,sizeof(libraw_internal_data.internal_output_params));

    SET_PROC_FLAG(LIBRAW_PROGRESS_LOAD_RAW);
    RUN_CALLBACK(LIBRAW_PROGRESS_LOAD_RAW,1,2);

    return 0;
  }
  catch ( LibRaw_exceptions err) {
    EXCEPTION_HANDLER(err);
  }
  catch (std::exception ee) {
    EXCEPTION_HANDLER(LIBRAW_EXCEPTION_IO_CORRUPT);
  }
}