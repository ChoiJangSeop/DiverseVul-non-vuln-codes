TIFFWriteDirectorySec(TIFF* tif, int isimage, int imagedone, uint64* pdiroff)
{
	static const char module[] = "TIFFWriteDirectorySec";
	uint32 ndir;
	TIFFDirEntry* dir;
	uint32 dirsize;
	void* dirmem;
	uint32 m;
	if (tif->tif_mode == O_RDONLY)
		return (1);

        _TIFFFillStriles( tif );
        
	/*
	 * Clear write state so that subsequent images with
	 * different characteristics get the right buffers
	 * setup for them.
	 */
	if (imagedone)
	{
		if (tif->tif_flags & TIFF_POSTENCODE)
		{
			tif->tif_flags &= ~TIFF_POSTENCODE;
			if (!(*tif->tif_postencode)(tif))
			{
				TIFFErrorExt(tif->tif_clientdata,module,
				    "Error post-encoding before directory write");
				return (0);
			}
		}
		(*tif->tif_close)(tif);       /* shutdown encoder */
		/*
		 * Flush any data that might have been written
		 * by the compression close+cleanup routines.  But
                 * be careful not to write stuff if we didn't add data
                 * in the previous steps as the "rawcc" data may well be
                 * a previously read tile/strip in mixed read/write mode.
		 */
		if (tif->tif_rawcc > 0 
		    && (tif->tif_flags & TIFF_BEENWRITING) != 0 )
		{
		    if( !TIFFFlushData1(tif) )
                    {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "Error flushing data before directory write");
			return (0);
                    }
		}
		if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata)
		{
			_TIFFfree(tif->tif_rawdata);
			tif->tif_rawdata = NULL;
			tif->tif_rawcc = 0;
			tif->tif_rawdatasize = 0;
                        tif->tif_rawdataoff = 0;
                        tif->tif_rawdataloaded = 0;
		}
		tif->tif_flags &= ~(TIFF_BEENWRITING|TIFF_BUFFERSETUP);
	}
	dir=NULL;
	dirmem=NULL;
	dirsize=0;
	while (1)
	{
		ndir=0;
		if (isimage)
		{
			if (TIFFFieldSet(tif,FIELD_IMAGEDIMENSIONS))
			{
				if (!TIFFWriteDirectoryTagShortLong(tif,&ndir,dir,TIFFTAG_IMAGEWIDTH,tif->tif_dir.td_imagewidth))
					goto bad;
				if (!TIFFWriteDirectoryTagShortLong(tif,&ndir,dir,TIFFTAG_IMAGELENGTH,tif->tif_dir.td_imagelength))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_TILEDIMENSIONS))
			{
				if (!TIFFWriteDirectoryTagShortLong(tif,&ndir,dir,TIFFTAG_TILEWIDTH,tif->tif_dir.td_tilewidth))
					goto bad;
				if (!TIFFWriteDirectoryTagShortLong(tif,&ndir,dir,TIFFTAG_TILELENGTH,tif->tif_dir.td_tilelength))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_RESOLUTION))
			{
				if (!TIFFWriteDirectoryTagRational(tif,&ndir,dir,TIFFTAG_XRESOLUTION,tif->tif_dir.td_xresolution))
					goto bad;
				if (!TIFFWriteDirectoryTagRational(tif,&ndir,dir,TIFFTAG_YRESOLUTION,tif->tif_dir.td_yresolution))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_POSITION))
			{
				if (!TIFFWriteDirectoryTagRational(tif,&ndir,dir,TIFFTAG_XPOSITION,tif->tif_dir.td_xposition))
					goto bad;
				if (!TIFFWriteDirectoryTagRational(tif,&ndir,dir,TIFFTAG_YPOSITION,tif->tif_dir.td_yposition))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_SUBFILETYPE))
			{
				if (!TIFFWriteDirectoryTagLong(tif,&ndir,dir,TIFFTAG_SUBFILETYPE,tif->tif_dir.td_subfiletype))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_BITSPERSAMPLE))
			{
				if (!TIFFWriteDirectoryTagShortPerSample(tif,&ndir,dir,TIFFTAG_BITSPERSAMPLE,tif->tif_dir.td_bitspersample))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_COMPRESSION))
			{
				if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_COMPRESSION,tif->tif_dir.td_compression))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_PHOTOMETRIC))
			{
				if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_PHOTOMETRIC,tif->tif_dir.td_photometric))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_THRESHHOLDING))
			{
				if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_THRESHHOLDING,tif->tif_dir.td_threshholding))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_FILLORDER))
			{
				if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_FILLORDER,tif->tif_dir.td_fillorder))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_ORIENTATION))
			{
				if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_ORIENTATION,tif->tif_dir.td_orientation))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_SAMPLESPERPIXEL))
			{
				if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_SAMPLESPERPIXEL,tif->tif_dir.td_samplesperpixel))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_ROWSPERSTRIP))
			{
				if (!TIFFWriteDirectoryTagShortLong(tif,&ndir,dir,TIFFTAG_ROWSPERSTRIP,tif->tif_dir.td_rowsperstrip))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_MINSAMPLEVALUE))
			{
				if (!TIFFWriteDirectoryTagShortPerSample(tif,&ndir,dir,TIFFTAG_MINSAMPLEVALUE,tif->tif_dir.td_minsamplevalue))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_MAXSAMPLEVALUE))
			{
				if (!TIFFWriteDirectoryTagShortPerSample(tif,&ndir,dir,TIFFTAG_MAXSAMPLEVALUE,tif->tif_dir.td_maxsamplevalue))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_PLANARCONFIG))
			{
				if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_PLANARCONFIG,tif->tif_dir.td_planarconfig))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_RESOLUTIONUNIT))
			{
				if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_RESOLUTIONUNIT,tif->tif_dir.td_resolutionunit))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_PAGENUMBER))
			{
				if (!TIFFWriteDirectoryTagShortArray(tif,&ndir,dir,TIFFTAG_PAGENUMBER,2,&tif->tif_dir.td_pagenumber[0]))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_STRIPBYTECOUNTS))
			{
				if (!isTiled(tif))
				{
					if (!TIFFWriteDirectoryTagLongLong8Array(tif,&ndir,dir,TIFFTAG_STRIPBYTECOUNTS,tif->tif_dir.td_nstrips,tif->tif_dir.td_stripbytecount))
						goto bad;
				}
				else
				{
					if (!TIFFWriteDirectoryTagLongLong8Array(tif,&ndir,dir,TIFFTAG_TILEBYTECOUNTS,tif->tif_dir.td_nstrips,tif->tif_dir.td_stripbytecount))
						goto bad;
				}
			}
			if (TIFFFieldSet(tif,FIELD_STRIPOFFSETS))
			{
				if (!isTiled(tif))
				{
                    /* td_stripoffset might be NULL in an odd OJPEG case. See
                     *  tif_dirread.c around line 3634.
                     * XXX: OJPEG hack.
                     * If a) compression is OJPEG, b) it's not a tiled TIFF,
                     * and c) the number of strips is 1,
                     * then we tolerate the absence of stripoffsets tag,
                     * because, presumably, all required data is in the
                     * JpegInterchangeFormat stream.
                     * We can get here when using tiffset on such a file.
                     * See http://bugzilla.maptools.org/show_bug.cgi?id=2500
                    */
                    if (tif->tif_dir.td_stripoffset != NULL &&
                        !TIFFWriteDirectoryTagLongLong8Array(tif,&ndir,dir,TIFFTAG_STRIPOFFSETS,tif->tif_dir.td_nstrips,tif->tif_dir.td_stripoffset))
                        goto bad;
				}
				else
				{
					if (!TIFFWriteDirectoryTagLongLong8Array(tif,&ndir,dir,TIFFTAG_TILEOFFSETS,tif->tif_dir.td_nstrips,tif->tif_dir.td_stripoffset))
						goto bad;
				}
			}
			if (TIFFFieldSet(tif,FIELD_COLORMAP))
			{
				if (!TIFFWriteDirectoryTagColormap(tif,&ndir,dir))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_EXTRASAMPLES))
			{
				if (tif->tif_dir.td_extrasamples)
				{
					uint16 na;
					uint16* nb;
					TIFFGetFieldDefaulted(tif,TIFFTAG_EXTRASAMPLES,&na,&nb);
					if (!TIFFWriteDirectoryTagShortArray(tif,&ndir,dir,TIFFTAG_EXTRASAMPLES,na,nb))
						goto bad;
				}
			}
			if (TIFFFieldSet(tif,FIELD_SAMPLEFORMAT))
			{
				if (!TIFFWriteDirectoryTagShortPerSample(tif,&ndir,dir,TIFFTAG_SAMPLEFORMAT,tif->tif_dir.td_sampleformat))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_SMINSAMPLEVALUE))
			{
				if (!TIFFWriteDirectoryTagSampleformatArray(tif,&ndir,dir,TIFFTAG_SMINSAMPLEVALUE,tif->tif_dir.td_samplesperpixel,tif->tif_dir.td_sminsamplevalue))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_SMAXSAMPLEVALUE))
			{
				if (!TIFFWriteDirectoryTagSampleformatArray(tif,&ndir,dir,TIFFTAG_SMAXSAMPLEVALUE,tif->tif_dir.td_samplesperpixel,tif->tif_dir.td_smaxsamplevalue))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_IMAGEDEPTH))
			{
				if (!TIFFWriteDirectoryTagLong(tif,&ndir,dir,TIFFTAG_IMAGEDEPTH,tif->tif_dir.td_imagedepth))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_TILEDEPTH))
			{
				if (!TIFFWriteDirectoryTagLong(tif,&ndir,dir,TIFFTAG_TILEDEPTH,tif->tif_dir.td_tiledepth))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_HALFTONEHINTS))
			{
				if (!TIFFWriteDirectoryTagShortArray(tif,&ndir,dir,TIFFTAG_HALFTONEHINTS,2,&tif->tif_dir.td_halftonehints[0]))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_YCBCRSUBSAMPLING))
			{
				if (!TIFFWriteDirectoryTagShortArray(tif,&ndir,dir,TIFFTAG_YCBCRSUBSAMPLING,2,&tif->tif_dir.td_ycbcrsubsampling[0]))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_YCBCRPOSITIONING))
			{
				if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_YCBCRPOSITIONING,tif->tif_dir.td_ycbcrpositioning))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_REFBLACKWHITE))
			{
				if (!TIFFWriteDirectoryTagRationalArray(tif,&ndir,dir,TIFFTAG_REFERENCEBLACKWHITE,6,tif->tif_dir.td_refblackwhite))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_TRANSFERFUNCTION))
			{
				if (!TIFFWriteDirectoryTagTransferfunction(tif,&ndir,dir))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_INKNAMES))
			{
				if (!TIFFWriteDirectoryTagAscii(tif,&ndir,dir,TIFFTAG_INKNAMES,tif->tif_dir.td_inknameslen,tif->tif_dir.td_inknames))
					goto bad;
			}
			if (TIFFFieldSet(tif,FIELD_SUBIFD))
			{
				if (!TIFFWriteDirectoryTagSubifd(tif,&ndir,dir))
					goto bad;
			}
			{
				uint32 n;
				for (n=0; n<tif->tif_nfields; n++) {
					const TIFFField* o;
					o = tif->tif_fields[n];
					if ((o->field_bit>=FIELD_CODEC)&&(TIFFFieldSet(tif,o->field_bit)))
					{
						switch (o->get_field_type)
						{
							case TIFF_SETGET_ASCII:
								{
									uint32 pa;
									char* pb;
									assert(o->field_type==TIFF_ASCII);
									assert(o->field_readcount==TIFF_VARIABLE);
									assert(o->field_passcount==0);
									TIFFGetField(tif,o->field_tag,&pb);
									pa=(uint32)(strlen(pb));
									if (!TIFFWriteDirectoryTagAscii(tif,&ndir,dir,(uint16)o->field_tag,pa,pb))
										goto bad;
								}
								break;
							case TIFF_SETGET_UINT16:
								{
									uint16 p;
									assert(o->field_type==TIFF_SHORT);
									assert(o->field_readcount==1);
									assert(o->field_passcount==0);
									TIFFGetField(tif,o->field_tag,&p);
									if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,(uint16)o->field_tag,p))
										goto bad;
								}
								break;
							case TIFF_SETGET_UINT32:
								{
									uint32 p;
									assert(o->field_type==TIFF_LONG);
									assert(o->field_readcount==1);
									assert(o->field_passcount==0);
									TIFFGetField(tif,o->field_tag,&p);
									if (!TIFFWriteDirectoryTagLong(tif,&ndir,dir,(uint16)o->field_tag,p))
										goto bad;
								}
								break;
							case TIFF_SETGET_C32_UINT8:
								{
									uint32 pa;
									void* pb;
									assert(o->field_type==TIFF_UNDEFINED);
									assert(o->field_readcount==TIFF_VARIABLE2);
									assert(o->field_passcount==1);
									TIFFGetField(tif,o->field_tag,&pa,&pb);
									if (!TIFFWriteDirectoryTagUndefinedArray(tif,&ndir,dir,(uint16)o->field_tag,pa,pb))
										goto bad;
								}
								break;
							default:
								assert(0);   /* we should never get here */
								break;
						}
					}
				}
			}
		}
		for (m=0; m<(uint32)(tif->tif_dir.td_customValueCount); m++)
		{
                        uint16 tag = (uint16)tif->tif_dir.td_customValues[m].info->field_tag;
                        uint32 count = tif->tif_dir.td_customValues[m].count;
			switch (tif->tif_dir.td_customValues[m].info->field_type)
			{
				case TIFF_ASCII:
					if (!TIFFWriteDirectoryTagAscii(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_UNDEFINED:
					if (!TIFFWriteDirectoryTagUndefinedArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_BYTE:
					if (!TIFFWriteDirectoryTagByteArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SBYTE:
					if (!TIFFWriteDirectoryTagSbyteArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SHORT:
					if (!TIFFWriteDirectoryTagShortArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SSHORT:
					if (!TIFFWriteDirectoryTagSshortArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_LONG:
					if (!TIFFWriteDirectoryTagLongArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SLONG:
					if (!TIFFWriteDirectoryTagSlongArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_LONG8:
					if (!TIFFWriteDirectoryTagLong8Array(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SLONG8:
					if (!TIFFWriteDirectoryTagSlong8Array(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_RATIONAL:
					if (!TIFFWriteDirectoryTagRationalArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SRATIONAL:
					if (!TIFFWriteDirectoryTagSrationalArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_FLOAT:
					if (!TIFFWriteDirectoryTagFloatArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_DOUBLE:
					if (!TIFFWriteDirectoryTagDoubleArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_IFD:
					if (!TIFFWriteDirectoryTagIfdArray(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_IFD8:
					if (!TIFFWriteDirectoryTagIfdIfd8Array(tif,&ndir,dir,tag,count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				default:
					assert(0);   /* we should never get here */
					break;
			}
		}
		if (dir!=NULL)
			break;
		dir=_TIFFmalloc(ndir*sizeof(TIFFDirEntry));
		if (dir==NULL)
		{
			TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
			goto bad;
		}
		if (isimage)
		{
			if ((tif->tif_diroff==0)&&(!TIFFLinkDirectory(tif)))
				goto bad;
		}
		else
			tif->tif_diroff=(TIFFSeekFile(tif,0,SEEK_END)+1)&(~((toff_t)1));
		if (pdiroff!=NULL)
			*pdiroff=tif->tif_diroff;
		if (!(tif->tif_flags&TIFF_BIGTIFF))
			dirsize=2+ndir*12+4;
		else
			dirsize=8+ndir*20+8;
		tif->tif_dataoff=tif->tif_diroff+dirsize;
		if (!(tif->tif_flags&TIFF_BIGTIFF))
			tif->tif_dataoff=(uint32)tif->tif_dataoff;
		if ((tif->tif_dataoff<tif->tif_diroff)||(tif->tif_dataoff<(uint64)dirsize))
		{
			TIFFErrorExt(tif->tif_clientdata,module,"Maximum TIFF file size exceeded");
			goto bad;
		}
		if (tif->tif_dataoff&1)
			tif->tif_dataoff++;
		if (isimage)
			tif->tif_curdir++;
	}
	if (isimage)
	{
		if (TIFFFieldSet(tif,FIELD_SUBIFD)&&(tif->tif_subifdoff==0))
		{
			uint32 na;
			TIFFDirEntry* nb;
			for (na=0, nb=dir; ; na++, nb++)
			{
				if( na == ndir )
                                {
                                    TIFFErrorExt(tif->tif_clientdata,module,
                                                 "Cannot find SubIFD tag");
                                    goto bad;
                                }
				if (nb->tdir_tag==TIFFTAG_SUBIFD)
					break;
			}
			if (!(tif->tif_flags&TIFF_BIGTIFF))
				tif->tif_subifdoff=tif->tif_diroff+2+na*12+8;
			else
				tif->tif_subifdoff=tif->tif_diroff+8+na*20+12;
		}
	}
	dirmem=_TIFFmalloc(dirsize);
	if (dirmem==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		goto bad;
	}
	if (!(tif->tif_flags&TIFF_BIGTIFF))
	{
		uint8* n;
		uint32 nTmp;
		TIFFDirEntry* o;
		n=dirmem;
		*(uint16*)n=(uint16)ndir;
		if (tif->tif_flags&TIFF_SWAB)
			TIFFSwabShort((uint16*)n);
		n+=2;
		o=dir;
		for (m=0; m<ndir; m++)
		{
			*(uint16*)n=o->tdir_tag;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabShort((uint16*)n);
			n+=2;
			*(uint16*)n=o->tdir_type;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabShort((uint16*)n);
			n+=2;
			nTmp = (uint32)o->tdir_count;
			_TIFFmemcpy(n,&nTmp,4);
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabLong((uint32*)n);
			n+=4;
			/* This is correct. The data has been */
			/* swabbed previously in TIFFWriteDirectoryTagData */
			_TIFFmemcpy(n,&o->tdir_offset,4);
			n+=4;
			o++;
		}
		nTmp = (uint32)tif->tif_nextdiroff;
		if (tif->tif_flags&TIFF_SWAB)
			TIFFSwabLong(&nTmp);
		_TIFFmemcpy(n,&nTmp,4);
	}
	else
	{
		uint8* n;
		TIFFDirEntry* o;
		n=dirmem;
		*(uint64*)n=ndir;
		if (tif->tif_flags&TIFF_SWAB)
			TIFFSwabLong8((uint64*)n);
		n+=8;
		o=dir;
		for (m=0; m<ndir; m++)
		{
			*(uint16*)n=o->tdir_tag;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabShort((uint16*)n);
			n+=2;
			*(uint16*)n=o->tdir_type;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabShort((uint16*)n);
			n+=2;
			_TIFFmemcpy(n,&o->tdir_count,8);
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabLong8((uint64*)n);
			n+=8;
			_TIFFmemcpy(n,&o->tdir_offset,8);
			n+=8;
			o++;
		}
		_TIFFmemcpy(n,&tif->tif_nextdiroff,8);
		if (tif->tif_flags&TIFF_SWAB)
			TIFFSwabLong8((uint64*)n);
	}
	_TIFFfree(dir);
	dir=NULL;
	if (!SeekOK(tif,tif->tif_diroff))
	{
		TIFFErrorExt(tif->tif_clientdata,module,"IO error writing directory");
		goto bad;
	}
	if (!WriteOK(tif,dirmem,(tmsize_t)dirsize))
	{
		TIFFErrorExt(tif->tif_clientdata,module,"IO error writing directory");
		goto bad;
	}
	_TIFFfree(dirmem);
	if (imagedone)
	{
		TIFFFreeDirectory(tif);
		tif->tif_flags &= ~TIFF_DIRTYDIRECT;
		tif->tif_flags &= ~TIFF_DIRTYSTRIP;
		(*tif->tif_cleanup)(tif);
		/*
		* Reset directory-related state for subsequent
		* directories.
		*/
		TIFFCreateDirectory(tif);
	}
	return(1);
bad:
	if (dir!=NULL)
		_TIFFfree(dir);
	if (dirmem!=NULL)
		_TIFFfree(dirmem);
	return(0);
}