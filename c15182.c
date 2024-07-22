void run(char const* filename, char const* filter,
         bool encode, unsigned int columns,
         int bits_per_sample, int samples_per_pixel)
{
    FILE* in = QUtil::safe_fopen(filename, "rb");
    FILE* o1 = QUtil::safe_fopen("out", "wb");
    Pipeline* out = new Pl_StdioFile("out", o1);
    Pipeline* pl = 0;
    if (strcmp(filter, "png") == 0)
    {
        pl = new Pl_PNGFilter(
            "png", out,
            encode ? Pl_PNGFilter::a_encode : Pl_PNGFilter::a_decode,
            columns, samples_per_pixel, bits_per_sample);
    }
    else if (strcmp(filter, "tiff") == 0)
    {
        pl = new Pl_TIFFPredictor(
            "png", out,
            encode ? Pl_TIFFPredictor::a_encode : Pl_TIFFPredictor::a_decode,
            columns, samples_per_pixel, bits_per_sample);
    }
    else
    {
        std::cerr << "unknown filter " << filter << std::endl;
        exit(2);
    }
    assert((2 * (columns + 1)) < 1024);
    unsigned char buf[1024];
    size_t len;
    while (true)
    {
	len = fread(buf, 1, (2 * columns) + 1, in);
	if (len == 0)
	{
	    break;
	}
	pl->write(buf, len);
	len = fread(buf, 1, 1, in);
	if (len == 0)
	{
	    break;
	}
	pl->write(buf, len);
	len = fread(buf, 1, 1, in);
	if (len == 0)
	{
	    break;
	}
	pl->write(buf, len);
    }

    pl->finish();
    delete pl;
    delete out;
    fclose(o1);
    fclose(in);

    std::cout << "done" << std::endl;
}