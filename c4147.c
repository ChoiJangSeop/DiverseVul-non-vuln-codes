void esp_reg_write(ESPState *s, uint32_t saddr, uint64_t val)
{
    trace_esp_mem_writeb(saddr, s->wregs[saddr], val);
    switch (saddr) {
    case ESP_TCHI:
        s->tchi_written = true;
        /* fall through */
    case ESP_TCLO:
    case ESP_TCMID:
        s->rregs[ESP_RSTAT] &= ~STAT_TC;
        break;
    case ESP_FIFO:
        if (s->do_cmd) {
            if (s->cmdlen < TI_BUFSZ) {
                s->cmdbuf[s->cmdlen++] = val & 0xff;
            } else {
                trace_esp_error_fifo_overrun();
            }
        } else if (s->ti_wptr == TI_BUFSZ - 1) {
            trace_esp_error_fifo_overrun();
        } else {
            s->ti_size++;
            s->ti_buf[s->ti_wptr++] = val & 0xff;
        }
        break;
    case ESP_CMD:
        s->rregs[saddr] = val;
        if (val & CMD_DMA) {
            s->dma = 1;
            /* Reload DMA counter.  */
            s->rregs[ESP_TCLO] = s->wregs[ESP_TCLO];
            s->rregs[ESP_TCMID] = s->wregs[ESP_TCMID];
            s->rregs[ESP_TCHI] = s->wregs[ESP_TCHI];
        } else {
            s->dma = 0;
        }
        switch(val & CMD_CMD) {
        case CMD_NOP:
            trace_esp_mem_writeb_cmd_nop(val);
            break;
        case CMD_FLUSH:
            trace_esp_mem_writeb_cmd_flush(val);
            //s->ti_size = 0;
            s->rregs[ESP_RINTR] = INTR_FC;
            s->rregs[ESP_RSEQ] = 0;
            s->rregs[ESP_RFLAGS] = 0;
            break;
        case CMD_RESET:
            trace_esp_mem_writeb_cmd_reset(val);
            esp_soft_reset(s);
            break;
        case CMD_BUSRESET:
            trace_esp_mem_writeb_cmd_bus_reset(val);
            s->rregs[ESP_RINTR] = INTR_RST;
            if (!(s->wregs[ESP_CFG1] & CFG1_RESREPT)) {
                esp_raise_irq(s);
            }
            break;
        case CMD_TI:
            handle_ti(s);
            break;
        case CMD_ICCS:
            trace_esp_mem_writeb_cmd_iccs(val);
            write_response(s);
            s->rregs[ESP_RINTR] = INTR_FC;
            s->rregs[ESP_RSTAT] |= STAT_MI;
            break;
        case CMD_MSGACC:
            trace_esp_mem_writeb_cmd_msgacc(val);
            s->rregs[ESP_RINTR] = INTR_DC;
            s->rregs[ESP_RSEQ] = 0;
            s->rregs[ESP_RFLAGS] = 0;
            esp_raise_irq(s);
            break;
        case CMD_PAD:
            trace_esp_mem_writeb_cmd_pad(val);
            s->rregs[ESP_RSTAT] = STAT_TC;
            s->rregs[ESP_RINTR] = INTR_FC;
            s->rregs[ESP_RSEQ] = 0;
            break;
        case CMD_SATN:
            trace_esp_mem_writeb_cmd_satn(val);
            break;
        case CMD_RSTATN:
            trace_esp_mem_writeb_cmd_rstatn(val);
            break;
        case CMD_SEL:
            trace_esp_mem_writeb_cmd_sel(val);
            handle_s_without_atn(s);
            break;
        case CMD_SELATN:
            trace_esp_mem_writeb_cmd_selatn(val);
            handle_satn(s);
            break;
        case CMD_SELATNS:
            trace_esp_mem_writeb_cmd_selatns(val);
            handle_satn_stop(s);
            break;
        case CMD_ENSEL:
            trace_esp_mem_writeb_cmd_ensel(val);
            s->rregs[ESP_RINTR] = 0;
            break;
        case CMD_DISSEL:
            trace_esp_mem_writeb_cmd_dissel(val);
            s->rregs[ESP_RINTR] = 0;
            esp_raise_irq(s);
            break;
        default:
            trace_esp_error_unhandled_command(val);
            break;
        }
        break;
    case ESP_WBUSID ... ESP_WSYNO:
        break;
    case ESP_CFG1:
    case ESP_CFG2: case ESP_CFG3:
    case ESP_RES3: case ESP_RES4:
        s->rregs[saddr] = val;
        break;
    case ESP_WCCF ... ESP_WTEST:
        break;
    default:
        trace_esp_error_invalid_write(val, saddr);
        return;
    }
    s->wregs[saddr] = val;
}