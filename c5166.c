e1000e_write_packet_to_guest(E1000ECore *core, struct NetRxPkt *pkt,
                             const E1000E_RxRing *rxr,
                             const E1000E_RSSInfo *rss_info)
{
    PCIDevice *d = core->owner;
    dma_addr_t base;
    uint8_t desc[E1000_MAX_RX_DESC_LEN];
    size_t desc_size;
    size_t desc_offset = 0;
    size_t iov_ofs = 0;

    struct iovec *iov = net_rx_pkt_get_iovec(pkt);
    size_t size = net_rx_pkt_get_total_len(pkt);
    size_t total_size = size + e1000x_fcs_len(core->mac);
    const E1000E_RingInfo *rxi;
    size_t ps_hdr_len = 0;
    bool do_ps = e1000e_do_ps(core, pkt, &ps_hdr_len);
    bool is_first = true;

    rxi = rxr->i;

    do {
        hwaddr ba[MAX_PS_BUFFERS];
        e1000e_ba_state bastate = { { 0 } };
        bool is_last = false;

        desc_size = total_size - desc_offset;

        if (desc_size > core->rx_desc_buf_size) {
            desc_size = core->rx_desc_buf_size;
        }

        base = e1000e_ring_head_descr(core, rxi);

        pci_dma_read(d, base, &desc, core->rx_desc_len);

        trace_e1000e_rx_descr(rxi->idx, base, core->rx_desc_len);

        e1000e_read_rx_descr(core, desc, &ba);

        if (ba[0]) {
            if (desc_offset < size) {
                static const uint32_t fcs_pad;
                size_t iov_copy;
                size_t copy_size = size - desc_offset;
                if (copy_size > core->rx_desc_buf_size) {
                    copy_size = core->rx_desc_buf_size;
                }

                /* For PS mode copy the packet header first */
                if (do_ps) {
                    if (is_first) {
                        size_t ps_hdr_copied = 0;
                        do {
                            iov_copy = MIN(ps_hdr_len - ps_hdr_copied,
                                           iov->iov_len - iov_ofs);

                            e1000e_write_hdr_to_rx_buffers(core, &ba, &bastate,
                                                      iov->iov_base, iov_copy);

                            copy_size -= iov_copy;
                            ps_hdr_copied += iov_copy;

                            iov_ofs += iov_copy;
                            if (iov_ofs == iov->iov_len) {
                                iov++;
                                iov_ofs = 0;
                            }
                        } while (ps_hdr_copied < ps_hdr_len);

                        is_first = false;
                    } else {
                        /* Leave buffer 0 of each descriptor except first */
                        /* empty as per spec 7.1.5.1                      */
                        e1000e_write_hdr_to_rx_buffers(core, &ba, &bastate,
                                                       NULL, 0);
                    }
                }

                /* Copy packet payload */
                while (copy_size) {
                    iov_copy = MIN(copy_size, iov->iov_len - iov_ofs);

                    e1000e_write_to_rx_buffers(core, &ba, &bastate,
                                            iov->iov_base + iov_ofs, iov_copy);

                    copy_size -= iov_copy;
                    iov_ofs += iov_copy;
                    if (iov_ofs == iov->iov_len) {
                        iov++;
                        iov_ofs = 0;
                    }
                }

                if (desc_offset + desc_size >= total_size) {
                    /* Simulate FCS checksum presence in the last descriptor */
                    e1000e_write_to_rx_buffers(core, &ba, &bastate,
                          (const char *) &fcs_pad, e1000x_fcs_len(core->mac));
                }
            }
            desc_offset += desc_size;
            if (desc_offset >= total_size) {
                is_last = true;
            }
        } else { /* as per intel docs; skip descriptors with null buf addr */
            trace_e1000e_rx_null_descriptor();
        }

        e1000e_write_rx_descr(core, desc, is_last ? core->rx_pkt : NULL,
                           rss_info, do_ps ? ps_hdr_len : 0, &bastate.written);
        pci_dma_write(d, base, &desc, core->rx_desc_len);

        e1000e_ring_advance(core, rxi,
                            core->rx_desc_len / E1000_MIN_RX_DESC_LEN);

    } while (desc_offset < total_size);

    e1000e_update_rx_stats(core, size, total_size);
}