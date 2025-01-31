static void lsi_execute_script(LSIState *s)
{
    PCIDevice *pci_dev = PCI_DEVICE(s);
    uint32_t insn;
    uint32_t addr, addr_high;
    int opcode;
    int insn_processed = 0;

    s->istat1 |= LSI_ISTAT1_SRUN;
again:
    insn_processed++;
    insn = read_dword(s, s->dsp);
    if (!insn) {
        /* If we receive an empty opcode increment the DSP by 4 bytes
           instead of 8 and execute the next opcode at that location */
        s->dsp += 4;
        goto again;
    }
    addr = read_dword(s, s->dsp + 4);
    addr_high = 0;
    trace_lsi_execute_script(s->dsp, insn, addr);
    s->dsps = addr;
    s->dcmd = insn >> 24;
    s->dsp += 8;
    switch (insn >> 30) {
    case 0: /* Block move.  */
        if (s->sist1 & LSI_SIST1_STO) {
            trace_lsi_execute_script_blockmove_delayed();
            lsi_stop_script(s);
            break;
        }
        s->dbc = insn & 0xffffff;
        s->rbc = s->dbc;
        /* ??? Set ESA.  */
        s->ia = s->dsp - 8;
        if (insn & (1 << 29)) {
            /* Indirect addressing.  */
            addr = read_dword(s, addr);
        } else if (insn & (1 << 28)) {
            uint32_t buf[2];
            int32_t offset;
            /* Table indirect addressing.  */

            /* 32-bit Table indirect */
            offset = sextract32(addr, 0, 24);
            pci_dma_read(pci_dev, s->dsa + offset, buf, 8);
            /* byte count is stored in bits 0:23 only */
            s->dbc = cpu_to_le32(buf[0]) & 0xffffff;
            s->rbc = s->dbc;
            addr = cpu_to_le32(buf[1]);

            /* 40-bit DMA, upper addr bits [39:32] stored in first DWORD of
             * table, bits [31:24] */
            if (lsi_dma_40bit(s))
                addr_high = cpu_to_le32(buf[0]) >> 24;
            else if (lsi_dma_ti64bit(s)) {
                int selector = (cpu_to_le32(buf[0]) >> 24) & 0x1f;
                switch (selector) {
                case 0 ... 0x0f:
                    /* offset index into scratch registers since
                     * TI64 mode can use registers C to R */
                    addr_high = s->scratch[2 + selector];
                    break;
                case 0x10:
                    addr_high = s->mmrs;
                    break;
                case 0x11:
                    addr_high = s->mmws;
                    break;
                case 0x12:
                    addr_high = s->sfs;
                    break;
                case 0x13:
                    addr_high = s->drs;
                    break;
                case 0x14:
                    addr_high = s->sbms;
                    break;
                case 0x15:
                    addr_high = s->dbms;
                    break;
                default:
                    qemu_log_mask(LOG_GUEST_ERROR,
                          "lsi_scsi: Illegal selector specified (0x%x > 0x15) "
                          "for 64-bit DMA block move", selector);
                    break;
                }
            }
        } else if (lsi_dma_64bit(s)) {
            /* fetch a 3rd dword if 64-bit direct move is enabled and
               only if we're not doing table indirect or indirect addressing */
            s->dbms = read_dword(s, s->dsp);
            s->dsp += 4;
            s->ia = s->dsp - 12;
        }
        if ((s->sstat1 & PHASE_MASK) != ((insn >> 24) & 7)) {
            trace_lsi_execute_script_blockmove_badphase(
                    scsi_phase_name(s->sstat1),
                    scsi_phase_name(insn >> 24));
            lsi_script_scsi_interrupt(s, LSI_SIST0_MA, 0);
            break;
        }
        s->dnad = addr;
        s->dnad64 = addr_high;
        switch (s->sstat1 & 0x7) {
        case PHASE_DO:
            s->waiting = LSI_DMA_SCRIPTS;
            lsi_do_dma(s, 1);
            if (s->waiting)
                s->waiting = LSI_DMA_IN_PROGRESS;
            break;
        case PHASE_DI:
            s->waiting = LSI_DMA_SCRIPTS;
            lsi_do_dma(s, 0);
            if (s->waiting)
                s->waiting = LSI_DMA_IN_PROGRESS;
            break;
        case PHASE_CMD:
            lsi_do_command(s);
            break;
        case PHASE_ST:
            lsi_do_status(s);
            break;
        case PHASE_MO:
            lsi_do_msgout(s);
            break;
        case PHASE_MI:
            lsi_do_msgin(s);
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "lsi_scsi: Unimplemented phase %s\n",
                          scsi_phase_name(s->sstat1));
        }
        s->dfifo = s->dbc & 0xff;
        s->ctest5 = (s->ctest5 & 0xfc) | ((s->dbc >> 8) & 3);
        s->sbc = s->dbc;
        s->rbc -= s->dbc;
        s->ua = addr + s->dbc;
        break;

    case 1: /* IO or Read/Write instruction.  */
        opcode = (insn >> 27) & 7;
        if (opcode < 5) {
            uint32_t id;

            if (insn & (1 << 25)) {
                id = read_dword(s, s->dsa + sextract32(insn, 0, 24));
            } else {
                id = insn;
            }
            id = (id >> 16) & 0xf;
            if (insn & (1 << 26)) {
                addr = s->dsp + sextract32(addr, 0, 24);
            }
            s->dnad = addr;
            switch (opcode) {
            case 0: /* Select */
                s->sdid = id;
                if (s->scntl1 & LSI_SCNTL1_CON) {
                    trace_lsi_execute_script_io_alreadyreselected();
                    s->dsp = s->dnad;
                    break;
                }
                s->sstat0 |= LSI_SSTAT0_WOA;
                s->scntl1 &= ~LSI_SCNTL1_IARB;
                if (!scsi_device_find(&s->bus, 0, id, 0)) {
                    lsi_bad_selection(s, id);
                    break;
                }
                trace_lsi_execute_script_io_selected(id,
                                             insn & (1 << 3) ? " ATN" : "");
                /* ??? Linux drivers compain when this is set.  Maybe
                   it only applies in low-level mode (unimplemented).
                lsi_script_scsi_interrupt(s, LSI_SIST0_CMP, 0); */
                s->select_tag = id << 8;
                s->scntl1 |= LSI_SCNTL1_CON;
                if (insn & (1 << 3)) {
                    s->socl |= LSI_SOCL_ATN;
                    s->sbcl |= LSI_SBCL_ATN;
                }
                s->sbcl |= LSI_SBCL_BSY;
                lsi_set_phase(s, PHASE_MO);
                s->waiting = LSI_NOWAIT;
                break;
            case 1: /* Disconnect */
                trace_lsi_execute_script_io_disconnect();
                s->scntl1 &= ~LSI_SCNTL1_CON;
                /* FIXME: this is not entirely correct; the target need not ask
                 * for reselection until it has to send data, while here we force a
                 * reselection as soon as the bus is free.  The correct flow would
                 * reselect before lsi_transfer_data and disconnect as soon as
                 * DMA ends.
                 */
                if (!s->current) {
                    lsi_request *p = get_pending_req(s);
                    if (p) {
                        lsi_reselect(s, p);
                    }
                }
                break;
            case 2: /* Wait Reselect */
                if (s->istat0 & LSI_ISTAT0_SIGP) {
                    s->dsp = s->dnad;
                } else if (!lsi_irq_on_rsl(s)) {
                        lsi_wait_reselect(s);
                }
                break;
            case 3: /* Set */
                trace_lsi_execute_script_io_set(
                        insn & (1 << 3) ? " ATN" : "",
                        insn & (1 << 6) ? " ACK" : "",
                        insn & (1 << 9) ? " TM" : "",
                        insn & (1 << 10) ? " CC" : "");
                if (insn & (1 << 3)) {
                    s->socl |= LSI_SOCL_ATN;
                    s->sbcl |= LSI_SBCL_ATN;
                    lsi_set_phase(s, PHASE_MO);
                }

                if (insn & (1 << 6)) {
                    s->sbcl |= LSI_SBCL_ACK;
                }

                if (insn & (1 << 9)) {
                    qemu_log_mask(LOG_UNIMP,
                        "lsi_scsi: Target mode not implemented\n");
                }
                if (insn & (1 << 10))
                    s->carry = 1;
                break;
            case 4: /* Clear */
                trace_lsi_execute_script_io_clear(
                        insn & (1 << 3) ? " ATN" : "",
                        insn & (1 << 6) ? " ACK" : "",
                        insn & (1 << 9) ? " TM" : "",
                        insn & (1 << 10) ? " CC" : "");
                if (insn & (1 << 3)) {
                    s->socl &= ~LSI_SOCL_ATN;
                    s->sbcl &= ~LSI_SBCL_ATN;
                }

                if (insn & (1 << 6)) {
                    s->sbcl &= ~LSI_SBCL_ACK;
                }

                if (insn & (1 << 10))
                    s->carry = 0;
                break;
            }
        } else {
            uint8_t op0;
            uint8_t op1;
            uint8_t data8;
            int reg;
            int operator;

            static const char *opcode_names[3] =
                {"Write", "Read", "Read-Modify-Write"};
            static const char *operator_names[8] =
                {"MOV", "SHL", "OR", "XOR", "AND", "SHR", "ADD", "ADC"};

            reg = ((insn >> 16) & 0x7f) | (insn & 0x80);
            data8 = (insn >> 8) & 0xff;
            opcode = (insn >> 27) & 7;
            operator = (insn >> 24) & 7;
            trace_lsi_execute_script_io_opcode(
                    opcode_names[opcode - 5], reg,
                    operator_names[operator], data8, s->sfbr,
                    (insn & (1 << 23)) ? " SFBR" : "");
            op0 = op1 = 0;
            switch (opcode) {
            case 5: /* From SFBR */
                op0 = s->sfbr;
                op1 = data8;
                break;
            case 6: /* To SFBR */
                if (operator)
                    op0 = lsi_reg_readb(s, reg);
                op1 = data8;
                break;
            case 7: /* Read-modify-write */
                if (operator)
                    op0 = lsi_reg_readb(s, reg);
                if (insn & (1 << 23)) {
                    op1 = s->sfbr;
                } else {
                    op1 = data8;
                }
                break;
            }

            switch (operator) {
            case 0: /* move */
                op0 = op1;
                break;
            case 1: /* Shift left */
                op1 = op0 >> 7;
                op0 = (op0 << 1) | s->carry;
                s->carry = op1;
                break;
            case 2: /* OR */
                op0 |= op1;
                break;
            case 3: /* XOR */
                op0 ^= op1;
                break;
            case 4: /* AND */
                op0 &= op1;
                break;
            case 5: /* SHR */
                op1 = op0 & 1;
                op0 = (op0 >> 1) | (s->carry << 7);
                s->carry = op1;
                break;
            case 6: /* ADD */
                op0 += op1;
                s->carry = op0 < op1;
                break;
            case 7: /* ADC */
                op0 += op1 + s->carry;
                if (s->carry)
                    s->carry = op0 <= op1;
                else
                    s->carry = op0 < op1;
                break;
            }

            switch (opcode) {
            case 5: /* From SFBR */
            case 7: /* Read-modify-write */
                lsi_reg_writeb(s, reg, op0);
                break;
            case 6: /* To SFBR */
                s->sfbr = op0;
                break;
            }
        }
        break;

    case 2: /* Transfer Control.  */
        {
            int cond;
            int jmp;

            if ((insn & 0x002e0000) == 0) {
                trace_lsi_execute_script_tc_nop();
                break;
            }
            if (s->sist1 & LSI_SIST1_STO) {
                trace_lsi_execute_script_tc_delayedselect_timeout();
                lsi_stop_script(s);
                break;
            }
            cond = jmp = (insn & (1 << 19)) != 0;
            if (cond == jmp && (insn & (1 << 21))) {
                trace_lsi_execute_script_tc_compc(s->carry == jmp);
                cond = s->carry != 0;
            }
            if (cond == jmp && (insn & (1 << 17))) {
                trace_lsi_execute_script_tc_compp(scsi_phase_name(s->sstat1),
                        jmp ? '=' : '!', scsi_phase_name(insn >> 24));
                cond = (s->sstat1 & PHASE_MASK) == ((insn >> 24) & 7);
            }
            if (cond == jmp && (insn & (1 << 18))) {
                uint8_t mask;

                mask = (~insn >> 8) & 0xff;
                trace_lsi_execute_script_tc_compd(
                        s->sfbr, mask, jmp ? '=' : '!', insn & mask);
                cond = (s->sfbr & mask) == (insn & mask);
            }
            if (cond == jmp) {
                if (insn & (1 << 23)) {
                    /* Relative address.  */
                    addr = s->dsp + sextract32(addr, 0, 24);
                }
                switch ((insn >> 27) & 7) {
                case 0: /* Jump */
                    trace_lsi_execute_script_tc_jump(addr);
                    s->adder = addr;
                    s->dsp = addr;
                    break;
                case 1: /* Call */
                    trace_lsi_execute_script_tc_call(addr);
                    s->temp = s->dsp;
                    s->dsp = addr;
                    break;
                case 2: /* Return */
                    trace_lsi_execute_script_tc_return(s->temp);
                    s->dsp = s->temp;
                    break;
                case 3: /* Interrupt */
                    trace_lsi_execute_script_tc_interrupt(s->dsps);
                    if ((insn & (1 << 20)) != 0) {
                        s->istat0 |= LSI_ISTAT0_INTF;
                        lsi_update_irq(s);
                    } else {
                        lsi_script_dma_interrupt(s, LSI_DSTAT_SIR);
                    }
                    break;
                default:
                    trace_lsi_execute_script_tc_illegal();
                    lsi_script_dma_interrupt(s, LSI_DSTAT_IID);
                    break;
                }
            } else {
                trace_lsi_execute_script_tc_cc_failed();
            }
        }
        break;

    case 3:
        if ((insn & (1 << 29)) == 0) {
            /* Memory move.  */
            uint32_t dest;
            /* ??? The docs imply the destination address is loaded into
               the TEMP register.  However the Linux drivers rely on
               the value being presrved.  */
            dest = read_dword(s, s->dsp);
            s->dsp += 4;
            lsi_memcpy(s, dest, addr, insn & 0xffffff);
        } else {
            uint8_t data[7];
            int reg;
            int n;
            int i;

            if (insn & (1 << 28)) {
                addr = s->dsa + sextract32(addr, 0, 24);
            }
            n = (insn & 7);
            reg = (insn >> 16) & 0xff;
            if (insn & (1 << 24)) {
                pci_dma_read(pci_dev, addr, data, n);
                trace_lsi_execute_script_mm_load(reg, n, addr, *(int *)data);
                for (i = 0; i < n; i++) {
                    lsi_reg_writeb(s, reg + i, data[i]);
                }
            } else {
                trace_lsi_execute_script_mm_store(reg, n, addr);
                for (i = 0; i < n; i++) {
                    data[i] = lsi_reg_readb(s, reg + i);
                }
                pci_dma_write(pci_dev, addr, data, n);
            }
        }
    }
    if (insn_processed > 10000 && s->waiting == LSI_NOWAIT) {
        /* Some windows drivers make the device spin waiting for a memory
           location to change.  If we have been executed a lot of code then
           assume this is the case and force an unexpected device disconnect.
           This is apparently sufficient to beat the drivers into submission.
         */
        if (!(s->sien0 & LSI_SIST0_UDC)) {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "lsi_scsi: inf. loop with UDC masked");
        }
        lsi_script_scsi_interrupt(s, LSI_SIST0_UDC, 0);
        lsi_disconnect(s);
    } else if (s->istat1 & LSI_ISTAT1_SRUN && s->waiting == LSI_NOWAIT) {
        if (s->dcntl & LSI_DCNTL_SSM) {
            lsi_script_dma_interrupt(s, LSI_DSTAT_SSI);
        } else {
            goto again;
        }
    }
    trace_lsi_execute_script_stop();
}