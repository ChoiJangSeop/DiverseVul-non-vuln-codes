static unsigned xen_netbk_tx_build_gops(struct xen_netbk *netbk)
{
	struct gnttab_copy *gop = netbk->tx_copy_ops, *request_gop;
	struct sk_buff *skb;
	int ret;

	while (((nr_pending_reqs(netbk) + MAX_SKB_FRAGS) < MAX_PENDING_REQS) &&
		!list_empty(&netbk->net_schedule_list)) {
		struct xenvif *vif;
		struct xen_netif_tx_request txreq;
		struct xen_netif_tx_request txfrags[MAX_SKB_FRAGS];
		struct page *page;
		struct xen_netif_extra_info extras[XEN_NETIF_EXTRA_TYPE_MAX-1];
		u16 pending_idx;
		RING_IDX idx;
		int work_to_do;
		unsigned int data_len;
		pending_ring_idx_t index;

		/* Get a netif from the list with work to do. */
		vif = poll_net_schedule_list(netbk);
		if (!vif)
			continue;

		RING_FINAL_CHECK_FOR_REQUESTS(&vif->tx, work_to_do);
		if (!work_to_do) {
			xenvif_put(vif);
			continue;
		}

		idx = vif->tx.req_cons;
		rmb(); /* Ensure that we see the request before we copy it. */
		memcpy(&txreq, RING_GET_REQUEST(&vif->tx, idx), sizeof(txreq));

		/* Credit-based scheduling. */
		if (txreq.size > vif->remaining_credit &&
		    tx_credit_exceeded(vif, txreq.size)) {
			xenvif_put(vif);
			continue;
		}

		vif->remaining_credit -= txreq.size;

		work_to_do--;
		vif->tx.req_cons = ++idx;

		memset(extras, 0, sizeof(extras));
		if (txreq.flags & XEN_NETTXF_extra_info) {
			work_to_do = xen_netbk_get_extras(vif, extras,
							  work_to_do);
			idx = vif->tx.req_cons;
			if (unlikely(work_to_do < 0)) {
				netbk_tx_err(vif, &txreq, idx);
				continue;
			}
		}

		ret = netbk_count_requests(vif, &txreq, txfrags, work_to_do);
		if (unlikely(ret < 0)) {
			netbk_tx_err(vif, &txreq, idx - ret);
			continue;
		}
		idx += ret;

		if (unlikely(txreq.size < ETH_HLEN)) {
			netdev_dbg(vif->dev,
				   "Bad packet size: %d\n", txreq.size);
			netbk_tx_err(vif, &txreq, idx);
			continue;
		}

		/* No crossing a page as the payload mustn't fragment. */
		if (unlikely((txreq.offset + txreq.size) > PAGE_SIZE)) {
			netdev_dbg(vif->dev,
				   "txreq.offset: %x, size: %u, end: %lu\n",
				   txreq.offset, txreq.size,
				   (txreq.offset&~PAGE_MASK) + txreq.size);
			netbk_tx_err(vif, &txreq, idx);
			continue;
		}

		index = pending_index(netbk->pending_cons);
		pending_idx = netbk->pending_ring[index];

		data_len = (txreq.size > PKT_PROT_LEN &&
			    ret < MAX_SKB_FRAGS) ?
			PKT_PROT_LEN : txreq.size;

		skb = alloc_skb(data_len + NET_SKB_PAD + NET_IP_ALIGN,
				GFP_ATOMIC | __GFP_NOWARN);
		if (unlikely(skb == NULL)) {
			netdev_dbg(vif->dev,
				   "Can't allocate a skb in start_xmit.\n");
			netbk_tx_err(vif, &txreq, idx);
			break;
		}

		/* Packets passed to netif_rx() must have some headroom. */
		skb_reserve(skb, NET_SKB_PAD + NET_IP_ALIGN);

		if (extras[XEN_NETIF_EXTRA_TYPE_GSO - 1].type) {
			struct xen_netif_extra_info *gso;
			gso = &extras[XEN_NETIF_EXTRA_TYPE_GSO - 1];

			if (netbk_set_skb_gso(vif, skb, gso)) {
				kfree_skb(skb);
				netbk_tx_err(vif, &txreq, idx);
				continue;
			}
		}

		/* XXX could copy straight to head */
		page = xen_netbk_alloc_page(netbk, skb, pending_idx);
		if (!page) {
			kfree_skb(skb);
			netbk_tx_err(vif, &txreq, idx);
			continue;
		}

		gop->source.u.ref = txreq.gref;
		gop->source.domid = vif->domid;
		gop->source.offset = txreq.offset;

		gop->dest.u.gmfn = virt_to_mfn(page_address(page));
		gop->dest.domid = DOMID_SELF;
		gop->dest.offset = txreq.offset;

		gop->len = txreq.size;
		gop->flags = GNTCOPY_source_gref;

		gop++;

		memcpy(&netbk->pending_tx_info[pending_idx].req,
		       &txreq, sizeof(txreq));
		netbk->pending_tx_info[pending_idx].vif = vif;
		*((u16 *)skb->data) = pending_idx;

		__skb_put(skb, data_len);

		skb_shinfo(skb)->nr_frags = ret;
		if (data_len < txreq.size) {
			skb_shinfo(skb)->nr_frags++;
			frag_set_pending_idx(&skb_shinfo(skb)->frags[0],
					     pending_idx);
		} else {
			frag_set_pending_idx(&skb_shinfo(skb)->frags[0],
					     INVALID_PENDING_IDX);
		}

		netbk->pending_cons++;

		request_gop = xen_netbk_get_requests(netbk, vif,
						     skb, txfrags, gop);
		if (request_gop == NULL) {
			kfree_skb(skb);
			netbk_tx_err(vif, &txreq, idx);
			continue;
		}
		gop = request_gop;

		__skb_queue_tail(&netbk->tx_queue, skb);

		vif->tx.req_cons = idx;
		xen_netbk_check_rx_xenvif(vif);

		if ((gop-netbk->tx_copy_ops) >= ARRAY_SIZE(netbk->tx_copy_ops))
			break;
	}

	return gop - netbk->tx_copy_ops;
}