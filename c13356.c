static void free_huge_page(struct page *page)
{
	/*
	 * Can't pass hstate in here because it is called from the
	 * compound page destructor.
	 */
	struct hstate *h = page_hstate(page);
	int nid = page_to_nid(page);
	struct address_space *mapping;

	mapping = (struct address_space *) page_private(page);
	set_page_private(page, 0);
	page->mapping = NULL;
	BUG_ON(page_count(page));
	BUG_ON(page_mapcount(page));
	INIT_LIST_HEAD(&page->lru);

	spin_lock(&hugetlb_lock);
	if (h->surplus_huge_pages_node[nid] && huge_page_order(h) < MAX_ORDER) {
		update_and_free_page(h, page);
		h->surplus_huge_pages--;
		h->surplus_huge_pages_node[nid]--;
	} else {
		enqueue_huge_page(h, page);
	}
	spin_unlock(&hugetlb_lock);
	if (mapping)
		hugetlb_put_quota(mapping, 1);
}