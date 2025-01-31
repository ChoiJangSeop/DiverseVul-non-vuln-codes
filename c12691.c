static void collapse_huge_page(struct mm_struct *mm,
			       unsigned long address,
			       struct page **hpage,
			       struct vm_area_struct *vma,
			       int node)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd, _pmd;
	pte_t *pte;
	pgtable_t pgtable;
	struct page *new_page;
	spinlock_t *ptl;
	int isolated;
	unsigned long hstart, hend;

	VM_BUG_ON(address & ~HPAGE_PMD_MASK);
#ifndef CONFIG_NUMA
	VM_BUG_ON(!*hpage);
	new_page = *hpage;
	if (unlikely(mem_cgroup_newpage_charge(new_page, mm, GFP_KERNEL))) {
		up_read(&mm->mmap_sem);
		return;
	}
#else
	VM_BUG_ON(*hpage);
	/*
	 * Allocate the page while the vma is still valid and under
	 * the mmap_sem read mode so there is no memory allocation
	 * later when we take the mmap_sem in write mode. This is more
	 * friendly behavior (OTOH it may actually hide bugs) to
	 * filesystems in userland with daemons allocating memory in
	 * the userland I/O paths.  Allocating memory with the
	 * mmap_sem in read mode is good idea also to allow greater
	 * scalability.
	 */
	new_page = alloc_hugepage_vma(khugepaged_defrag(), vma, address,
				      node, __GFP_OTHER_NODE);
	if (unlikely(!new_page)) {
		up_read(&mm->mmap_sem);
		count_vm_event(THP_COLLAPSE_ALLOC_FAILED);
		*hpage = ERR_PTR(-ENOMEM);
		return;
	}
	count_vm_event(THP_COLLAPSE_ALLOC);
	if (unlikely(mem_cgroup_newpage_charge(new_page, mm, GFP_KERNEL))) {
		up_read(&mm->mmap_sem);
		put_page(new_page);
		return;
	}
#endif

	/* after allocating the hugepage upgrade to mmap_sem write mode */
	up_read(&mm->mmap_sem);

	/*
	 * Prevent all access to pagetables with the exception of
	 * gup_fast later hanlded by the ptep_clear_flush and the VM
	 * handled by the anon_vma lock + PG_lock.
	 */
	down_write(&mm->mmap_sem);
	if (unlikely(khugepaged_test_exit(mm)))
		goto out;

	vma = find_vma(mm, address);
	hstart = (vma->vm_start + ~HPAGE_PMD_MASK) & HPAGE_PMD_MASK;
	hend = vma->vm_end & HPAGE_PMD_MASK;
	if (address < hstart || address + HPAGE_PMD_SIZE > hend)
		goto out;

	if ((!(vma->vm_flags & VM_HUGEPAGE) && !khugepaged_always()) ||
	    (vma->vm_flags & VM_NOHUGEPAGE))
		goto out;

	/* VM_PFNMAP vmas may have vm_ops null but vm_file set */
	if (!vma->anon_vma || vma->vm_ops || vma->vm_file)
		goto out;
	if (is_vma_temporary_stack(vma))
		goto out;
	VM_BUG_ON(is_linear_pfn_mapping(vma) || is_pfn_mapping(vma));

	pgd = pgd_offset(mm, address);
	if (!pgd_present(*pgd))
		goto out;

	pud = pud_offset(pgd, address);
	if (!pud_present(*pud))
		goto out;

	pmd = pmd_offset(pud, address);
	/* pmd can't go away or become huge under us */
	if (!pmd_present(*pmd) || pmd_trans_huge(*pmd))
		goto out;

	anon_vma_lock(vma->anon_vma);

	pte = pte_offset_map(pmd, address);
	ptl = pte_lockptr(mm, pmd);

	spin_lock(&mm->page_table_lock); /* probably unnecessary */
	/*
	 * After this gup_fast can't run anymore. This also removes
	 * any huge TLB entry from the CPU so we won't allow
	 * huge and small TLB entries for the same virtual address
	 * to avoid the risk of CPU bugs in that area.
	 */
	_pmd = pmdp_clear_flush_notify(vma, address, pmd);
	spin_unlock(&mm->page_table_lock);

	spin_lock(ptl);
	isolated = __collapse_huge_page_isolate(vma, address, pte);
	spin_unlock(ptl);

	if (unlikely(!isolated)) {
		pte_unmap(pte);
		spin_lock(&mm->page_table_lock);
		BUG_ON(!pmd_none(*pmd));
		set_pmd_at(mm, address, pmd, _pmd);
		spin_unlock(&mm->page_table_lock);
		anon_vma_unlock(vma->anon_vma);
		goto out;
	}

	/*
	 * All pages are isolated and locked so anon_vma rmap
	 * can't run anymore.
	 */
	anon_vma_unlock(vma->anon_vma);

	__collapse_huge_page_copy(pte, new_page, vma, address, ptl);
	pte_unmap(pte);
	__SetPageUptodate(new_page);
	pgtable = pmd_pgtable(_pmd);
	VM_BUG_ON(page_count(pgtable) != 1);
	VM_BUG_ON(page_mapcount(pgtable) != 0);

	_pmd = mk_pmd(new_page, vma->vm_page_prot);
	_pmd = maybe_pmd_mkwrite(pmd_mkdirty(_pmd), vma);
	_pmd = pmd_mkhuge(_pmd);

	/*
	 * spin_lock() below is not the equivalent of smp_wmb(), so
	 * this is needed to avoid the copy_huge_page writes to become
	 * visible after the set_pmd_at() write.
	 */
	smp_wmb();

	spin_lock(&mm->page_table_lock);
	BUG_ON(!pmd_none(*pmd));
	page_add_new_anon_rmap(new_page, vma, address);
	set_pmd_at(mm, address, pmd, _pmd);
	update_mmu_cache(vma, address, entry);
	prepare_pmd_huge_pte(pgtable, mm);
	mm->nr_ptes--;
	spin_unlock(&mm->page_table_lock);

#ifndef CONFIG_NUMA
	*hpage = NULL;
#endif
	khugepaged_pages_collapsed++;
out_up_write:
	up_write(&mm->mmap_sem);
	return;

out:
	mem_cgroup_uncharge_page(new_page);
#ifdef CONFIG_NUMA
	put_page(new_page);
#endif
	goto out_up_write;
}