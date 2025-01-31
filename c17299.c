process(register int code, unsigned char** fill)
{
    int incode;
    static unsigned char firstchar;

    if (code == clear) {
	codesize = datasize + 1;
	codemask = (1 << codesize) - 1;
	avail = clear + 2;
	oldcode = -1;
	return 1;
    }

    if (oldcode == -1) {
	*(*fill)++ = suffix[code];
	firstchar = oldcode = code;
	return 1;
    }
    if (code > avail) {
	fprintf(stderr, "code %d too large for %d\n", code, avail);
	return 0; 
    }

    incode = code;
    if (code == avail) {      /* the first code is always < avail */
	*stackp++ = firstchar;
	code = oldcode;
    }
    while (code > clear) {
	*stackp++ = suffix[code];
	code = prefix[code];
    }

    *stackp++ = firstchar = suffix[code];
    prefix[avail] = oldcode;
    suffix[avail] = firstchar;
    avail++;

    if (((avail & codemask) == 0) && (avail < 4096)) {
	codesize++;
	codemask += avail;
    }
    oldcode = incode;
    do {
	*(*fill)++ = *--stackp;
    } while (stackp > stack);
    return 1;
}