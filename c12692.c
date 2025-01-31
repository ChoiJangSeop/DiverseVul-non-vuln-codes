int khugepaged_enter_vma_merge(struct vm_area_struct *vma)
{
	unsigned long hstart, hend;
	if (!vma->anon_vma)
		/*
		 * Not yet faulted in so we will register later in the
		 * page fault if needed.
		 */
		return 0;
	if (vma->vm_file || vma->vm_ops)
		/* khugepaged not yet working on file or special mappings */
		return 0;
	VM_BUG_ON(is_linear_pfn_mapping(vma) || is_pfn_mapping(vma));
	hstart = (vma->vm_start + ~HPAGE_PMD_MASK) & HPAGE_PMD_MASK;
	hend = vma->vm_end & HPAGE_PMD_MASK;
	if (hstart < hend)
		return khugepaged_enter(vma);
	return 0;
}