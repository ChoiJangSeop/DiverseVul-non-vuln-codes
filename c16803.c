OJPEGPreDecode(TIFF* tif, uint16 s)
{
	OJPEGState* sp=(OJPEGState*)tif->tif_data;
	uint32 m;
	if (sp->subsamplingcorrect_done==0)
		OJPEGSubsamplingCorrect(tif);
	if (sp->readheader_done==0)
	{
		if (OJPEGReadHeaderInfo(tif)==0)
			return(0);
	}
	if (sp->sos_end[s].log==0)
	{
		if (OJPEGReadSecondarySos(tif,s)==0)
			return(0);
	}
	if isTiled(tif)
		m=tif->tif_curtile;
	else
		m=tif->tif_curstrip;
	if ((sp->writeheader_done!=0) && ((sp->write_cursample!=s) || (sp->write_curstrile>m)))
	{
		if (sp->libjpeg_session_active!=0)
			OJPEGLibjpegSessionAbort(tif);
		sp->writeheader_done=0;
	}
	if (sp->writeheader_done==0)
	{
		sp->plane_sample_offset=(uint8)s;
		sp->write_cursample=s;
		sp->write_curstrile=s*tif->tif_dir.td_stripsperimage;
		if ((sp->in_buffer_file_pos_log==0) ||
		    (sp->in_buffer_file_pos-sp->in_buffer_togo!=sp->sos_end[s].in_buffer_file_pos))
		{
			sp->in_buffer_source=sp->sos_end[s].in_buffer_source;
			sp->in_buffer_next_strile=sp->sos_end[s].in_buffer_next_strile;
			sp->in_buffer_file_pos=sp->sos_end[s].in_buffer_file_pos;
			sp->in_buffer_file_pos_log=0;
			sp->in_buffer_file_togo=sp->sos_end[s].in_buffer_file_togo;
			sp->in_buffer_togo=0;
			sp->in_buffer_cur=0;
		}
		if (OJPEGWriteHeaderInfo(tif)==0)
			return(0);
	}
	while (sp->write_curstrile<m)          
	{
		if (sp->libjpeg_jpeg_query_style==0)
		{
			if (OJPEGPreDecodeSkipRaw(tif)==0)
				return(0);
		}
		else
		{
			if (OJPEGPreDecodeSkipScanlines(tif)==0)
				return(0);
		}
		sp->write_curstrile++;
	}
	return(1);
}