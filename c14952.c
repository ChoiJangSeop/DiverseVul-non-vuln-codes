static int mincore_unmapped_range(unsigned long addr, unsigned long end,
				   struct mm_walk *walk)
{
	walk->private += __mincore_unmapped_range(addr, end,
						  walk->vma, walk->private);
	return 0;
}