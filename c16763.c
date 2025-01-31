pickCopyFunc(TIFF* in, TIFF* out, uint16 bitspersample, uint16 samplesperpixel)
{
	uint16 shortv;
	uint32 w, l, tw, tl;
	int bychunk;

	(void) TIFFGetField(in, TIFFTAG_PLANARCONFIG, &shortv);
	if (shortv != config && bitspersample != 8 && samplesperpixel > 1) {
		fprintf(stderr,
		    "%s: Cannot handle different planar configuration w/ bits/sample != 8\n",
		    TIFFFileName(in));
		return (NULL);
	}
	TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(in, TIFFTAG_IMAGELENGTH, &l);
	if (!(TIFFIsTiled(out) || TIFFIsTiled(in))) {
		uint32 irps = (uint32) -1L;
		TIFFGetField(in, TIFFTAG_ROWSPERSTRIP, &irps);
		/* if biased, force decoded copying to allow image subtraction */
		bychunk = !bias && (rowsperstrip == irps);
	}else{  /* either in or out is tiled */
		if (bias) {
			fprintf(stderr,
			    "%s: Cannot handle tiled configuration w/bias image\n",
			TIFFFileName(in));
			return (NULL);
		}
		if (TIFFIsTiled(out)) {
			if (!TIFFGetField(in, TIFFTAG_TILEWIDTH, &tw))
				tw = w;
			if (!TIFFGetField(in, TIFFTAG_TILELENGTH, &tl))
				tl = l;
			bychunk = (tw == tilewidth && tl == tilelength);
		} else {  /* out's not, so in must be tiled */
			TIFFGetField(in, TIFFTAG_TILEWIDTH, &tw);
			TIFFGetField(in, TIFFTAG_TILELENGTH, &tl);
			bychunk = (tw == w && tl == rowsperstrip);
		}
	}
#define	T 1
#define	F 0
#define pack(a,b,c,d,e)	((long)(((a)<<11)|((b)<<3)|((c)<<2)|((d)<<1)|(e)))
	switch(pack(shortv,config,TIFFIsTiled(in),TIFFIsTiled(out),bychunk)) {
		/* Strips -> Tiles */
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   F,T,F):
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   F,T,T):
			return cpContigStrips2ContigTiles;
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, F,T,F):
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, F,T,T):
			return cpContigStrips2SeparateTiles;
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   F,T,F):
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   F,T,T):
			return cpSeparateStrips2ContigTiles;
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, F,T,F):
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, F,T,T):
			return cpSeparateStrips2SeparateTiles;
		/* Tiles -> Tiles */
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   T,T,F):
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   T,T,T):
			return cpContigTiles2ContigTiles;
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, T,T,F):
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, T,T,T):
			return cpContigTiles2SeparateTiles;
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   T,T,F):
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   T,T,T):
			return cpSeparateTiles2ContigTiles;
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, T,T,F):
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, T,T,T):
			return cpSeparateTiles2SeparateTiles;
		/* Tiles -> Strips */
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   T,F,F):
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   T,F,T):
			return cpContigTiles2ContigStrips;
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, T,F,F):
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, T,F,T):
			return cpContigTiles2SeparateStrips;
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   T,F,F):
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   T,F,T):
			return cpSeparateTiles2ContigStrips;
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, T,F,F):
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, T,F,T):
			return cpSeparateTiles2SeparateStrips;
		/* Strips -> Strips */
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   F,F,F):
			return bias ? cpBiasedContig2Contig : cpContig2ContigByRow;
		case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   F,F,T):
			return cpDecodedStrips;
		case pack(PLANARCONFIG_CONTIG, PLANARCONFIG_SEPARATE,   F,F,F):
		case pack(PLANARCONFIG_CONTIG, PLANARCONFIG_SEPARATE,   F,F,T):
			return cpContig2SeparateByRow;
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   F,F,F):
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   F,F,T):
			return cpSeparate2ContigByRow;
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, F,F,F):
		case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, F,F,T):
			return cpSeparate2SeparateByRow;
	}
#undef pack
#undef F
#undef T
	fprintf(stderr, "tiffcp: %s: Don't know how to copy/convert image.\n",
	    TIFFFileName(in));
	return (NULL);
}