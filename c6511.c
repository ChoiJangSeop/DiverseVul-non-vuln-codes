unsigned paravirt_patch_call(void *insnbuf,
			     const void *target, u16 tgt_clobbers,
			     unsigned long addr, u16 site_clobbers,
			     unsigned len)
{
	struct branch *b = insnbuf;
	unsigned long delta = (unsigned long)target - (addr+5);

	if (tgt_clobbers & ~site_clobbers)
		return len;	/* target would clobber too much for this site */
	if (len < 5)
		return len;	/* call too long for patch site */

	b->opcode = 0xe8; /* call */
	b->delta = delta;
	BUILD_BUG_ON(sizeof(*b) != 5);

	return 5;
}