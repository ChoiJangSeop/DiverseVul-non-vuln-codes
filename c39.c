int hash_page(unsigned long ea, unsigned long access, unsigned long trap)
{
	void *pgdir;
	unsigned long vsid;
	struct mm_struct *mm;
	pte_t *ptep;
	cpumask_t tmp;
	int rc, user_region = 0, local = 0;
	int psize;

	DBG_LOW("hash_page(ea=%016lx, access=%lx, trap=%lx\n",
		ea, access, trap);

	if ((ea & ~REGION_MASK) >= PGTABLE_RANGE) {
		DBG_LOW(" out of pgtable range !\n");
 		return 1;
	}

	/* Get region & vsid */
 	switch (REGION_ID(ea)) {
	case USER_REGION_ID:
		user_region = 1;
		mm = current->mm;
		if (! mm) {
			DBG_LOW(" user region with no mm !\n");
			return 1;
		}
		vsid = get_vsid(mm->context.id, ea);
		psize = mm->context.user_psize;
		break;
	case VMALLOC_REGION_ID:
		mm = &init_mm;
		vsid = get_kernel_vsid(ea);
		if (ea < VMALLOC_END)
			psize = mmu_vmalloc_psize;
		else
			psize = mmu_io_psize;
		break;
	default:
		/* Not a valid range
		 * Send the problem up to do_page_fault 
		 */
		return 1;
	}
	DBG_LOW(" mm=%p, mm->pgdir=%p, vsid=%016lx\n", mm, mm->pgd, vsid);

	/* Get pgdir */
	pgdir = mm->pgd;
	if (pgdir == NULL)
		return 1;

	/* Check CPU locality */
	tmp = cpumask_of_cpu(smp_processor_id());
	if (user_region && cpus_equal(mm->cpu_vm_mask, tmp))
		local = 1;

	/* Handle hugepage regions */
	if (unlikely(in_hugepage_area(mm->context, ea))) {
		DBG_LOW(" -> huge page !\n");
		return hash_huge_page(mm, access, ea, vsid, local, trap);
	}

	/* Get PTE and page size from page tables */
	ptep = find_linux_pte(pgdir, ea);
	if (ptep == NULL || !pte_present(*ptep)) {
		DBG_LOW(" no PTE !\n");
		return 1;
	}

#ifndef CONFIG_PPC_64K_PAGES
	DBG_LOW(" i-pte: %016lx\n", pte_val(*ptep));
#else
	DBG_LOW(" i-pte: %016lx %016lx\n", pte_val(*ptep),
		pte_val(*(ptep + PTRS_PER_PTE)));
#endif
	/* Pre-check access permissions (will be re-checked atomically
	 * in __hash_page_XX but this pre-check is a fast path
	 */
	if (access & ~pte_val(*ptep)) {
		DBG_LOW(" no access !\n");
		return 1;
	}

	/* Do actual hashing */
#ifndef CONFIG_PPC_64K_PAGES
	rc = __hash_page_4K(ea, access, vsid, ptep, trap, local);
#else
	if (mmu_ci_restrictions) {
		/* If this PTE is non-cacheable, switch to 4k */
		if (psize == MMU_PAGE_64K &&
		    (pte_val(*ptep) & _PAGE_NO_CACHE)) {
			if (user_region) {
				psize = MMU_PAGE_4K;
				mm->context.user_psize = MMU_PAGE_4K;
				mm->context.sllp = SLB_VSID_USER |
					mmu_psize_defs[MMU_PAGE_4K].sllp;
			} else if (ea < VMALLOC_END) {
				/*
				 * some driver did a non-cacheable mapping
				 * in vmalloc space, so switch vmalloc
				 * to 4k pages
				 */
				printk(KERN_ALERT "Reducing vmalloc segment "
				       "to 4kB pages because of "
				       "non-cacheable mapping\n");
				psize = mmu_vmalloc_psize = MMU_PAGE_4K;
			}
#ifdef CONFIG_SPE_BASE
			spu_flush_all_slbs(mm);
#endif
		}
		if (user_region) {
			if (psize != get_paca()->context.user_psize) {
				get_paca()->context = mm->context;
				slb_flush_and_rebolt();
			}
		} else if (get_paca()->vmalloc_sllp !=
			   mmu_psize_defs[mmu_vmalloc_psize].sllp) {
			get_paca()->vmalloc_sllp =
				mmu_psize_defs[mmu_vmalloc_psize].sllp;
			slb_flush_and_rebolt();
		}
	}
	if (psize == MMU_PAGE_64K)
		rc = __hash_page_64K(ea, access, vsid, ptep, trap, local);
	else
		rc = __hash_page_4K(ea, access, vsid, ptep, trap, local);
#endif /* CONFIG_PPC_64K_PAGES */

#ifndef CONFIG_PPC_64K_PAGES
	DBG_LOW(" o-pte: %016lx\n", pte_val(*ptep));
#else
	DBG_LOW(" o-pte: %016lx %016lx\n", pte_val(*ptep),
		pte_val(*(ptep + PTRS_PER_PTE)));
#endif
	DBG_LOW(" -> rc=%d\n", rc);
	return rc;
}