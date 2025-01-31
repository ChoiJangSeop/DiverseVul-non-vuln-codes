parse_wbxml_attribute_list (proto_tree *tree, tvbuff_t *tvb,
			    guint32 offset, guint32 str_tbl, guint8 level, guint8 *codepage_attr)
{
	guint32 tvb_len = tvb_reported_length (tvb);
	guint32 off     = offset;
	guint32 len;
	guint   str_len;
	guint32 ent;
	guint32 idx;
	guint8  peek;

	DebugLog(("parse_wbxml_attr (level = %u, offset = %u)\n", level, offset));
	/* Parse attributes */
	while (off < tvb_len) {
		peek = tvb_get_guint8 (tvb, off);
		DebugLog(("ATTR: (top of while) level = %3u, peek = 0x%02X, "
			  "off = %u, tvb_len = %u\n", level, peek, off, tvb_len));
		if ((peek & 0x3F) < 5) switch (peek) { /* Global tokens
							  in state = ATTR */
		case 0x00: /* SWITCH_PAGE */
			*codepage_attr = tvb_get_guint8 (tvb, off+1);
			proto_tree_add_text (tree, tvb, off, 2,
					     "      |  Attr | A -->%3d "
					     "| SWITCH_PAGE (Attr code page)    |",
					     *codepage_attr);
			off += 2;
			break;
		case 0x01: /* END */
			/* BEWARE
			 *   The Attribute END token means either ">" or "/>"
			 *   and as a consequence both must be treated separately.
			 *   This is done in the TAG state parser.
			 */
			off++;
			DebugLog(("ATTR: level = %u, Return: len = %u\n",
				  level, off - offset));
			return (off - offset);
		case 0x02: /* ENTITY */
			ent = tvb_get_guintvar (tvb, off+1, &len);
			proto_tree_add_text (tree, tvb, off, 1+len,
					     "  %3d |  Attr | A %3d    "
					     "| ENTITY                          "
					     "|     %s'&#%u;'",
					     level, *codepage_attr, Indent (level), ent);
			off += 1+len;
			break;
		case 0x03: /* STR_I */
			len = tvb_strsize (tvb, off+1);
			proto_tree_add_text (tree, tvb, off, 1+len,
					     "  %3d |  Attr | A %3d    "
					     "| STR_I (Inline string)           "
					     "|     %s\'%s\'",
					     level, *codepage_attr, Indent (level),
					     tvb_format_text (tvb, off+1, len-1));
			off += 1+len;
			break;
		case 0x04: /* LITERAL */
			idx = tvb_get_guintvar (tvb, off+1, &len);
			str_len = tvb_strsize (tvb, str_tbl+idx);
			proto_tree_add_text (tree, tvb, off, 1+len,
					     "  %3d |  Attr | A %3d    "
					     "| LITERAL (Literal Attribute)     "
					     "|   %s<%s />",
					     level, *codepage_attr, Indent (level),
					     tvb_format_text (tvb, str_tbl+idx, str_len-1));
			off += 1+len;
			break;
		case 0x40: /* EXT_I_0 */
		case 0x41: /* EXT_I_1 */
		case 0x42: /* EXT_I_2 */
			/* Extension tokens */
			len = tvb_strsize (tvb, off+1);
			proto_tree_add_text (tree, tvb, off, 1+len,
					     "  %3d |  Attr | A %3d    "
					     "| EXT_I_%1x    (Extension Token)    "
					     "|     %s(Inline string extension: \'%s\')",
					     level, *codepage_attr, peek & 0x0f, Indent (level),
					     tvb_format_text (tvb, off+1, len-1));
			off += 1+len;
			break;
			/* 0x43 impossible in ATTR state */
			/* 0x44 impossible in ATTR state */
		case 0x80: /* EXT_T_0 */
		case 0x81: /* EXT_T_1 */
		case 0x82: /* EXT_T_2 */
			/* Extension tokens */
			idx = tvb_get_guintvar (tvb, off+1, &len);
			proto_tree_add_text (tree, tvb, off, 1+len,
					     "  %3d |  Attr | A %3d    "
					     "| EXT_T_%1x    (Extension Token)    "
					     "|     %s(Extension Token, integer value: %u)",
					     level, *codepage_attr, peek & 0x0f, Indent (level),
					     idx);
			off += 1+len;
			break;
		case 0x83: /* STR_T */
			idx = tvb_get_guintvar (tvb, off+1, &len);
			str_len = tvb_strsize (tvb, str_tbl+idx);
			proto_tree_add_text (tree, tvb, off, 1+len,
					     "  %3d |  Attr | A %3d    "
					     "| STR_T (Tableref string)         "
					     "|     %s\'%s\'",
					     level, *codepage_attr, Indent (level),
					     tvb_format_text (tvb, str_tbl+idx, str_len-1));
			off += 1+len;
			break;
			/* 0x84 impossible in ATTR state */
		case 0xC0: /* EXT_0 */
		case 0xC1: /* EXT_1 */
		case 0xC2: /* EXT_2 */
			/* Extension tokens */
			proto_tree_add_text (tree, tvb, off, 1,
					     "  %3d |  Attr | A %3d    "
					     "| EXT_%1x      (Extension Token)    "
					     "|     %s(Single-byte extension)",
					     level, *codepage_attr, peek & 0x0f, Indent (level));
			off++;
			break;
		case 0xC3: /* OPAQUE - WBXML 1.1 and newer */
			if (tvb_get_guint8 (tvb, 0)) { /* WBXML 1.x (x > 0) */
				idx = tvb_get_guintvar (tvb, off+1, &len);
				proto_tree_add_text (tree, tvb, off, 1 + len + idx,
						     "  %3d |  Attr | A %3d    "
						     "| OPAQUE (Opaque data)            "
						     "|       %s(%d bytes of opaque data)",
						     level, *codepage_attr, Indent (level), idx);
				off += 1+len+idx;
			} else { /* WBXML 1.0 - RESERVED_2 token (invalid) */
				proto_tree_add_text (tree, tvb, off, 1,
						     "  %3d |  Attr | A %3d    "
						     "| RESERVED_2     (Invalid Token!) "
						     "| WBXML 1.0 parsing stops here.",
						     level, *codepage_attr);
				/* Stop processing as it is impossible to parse now */
				off = tvb_len;
				DebugLog(("ATTR: level = %u, Return: len = %u\n",
					  level, off - offset));
				return (off - offset);
			}
			break;
			/* 0xC4 impossible in ATTR state */
		default:
			proto_tree_add_text (tree, tvb, off, 1,
					     "  %3d |  Attr | A %3d    "
					     "| %-10s     (Invalid Token!) "
					     "| WBXML parsing stops here.",
					     level, *codepage_attr,
					     val_to_str_ext (peek, &vals_wbxml1x_global_tokens_ext, "(unknown 0x%x)"));
			/* Move to end of buffer */
			off = tvb_len;
			break;
		} else { /* Known atribute token */
			if (peek & 0x80) { /* attrValue */
				proto_tree_add_text (tree, tvb, off, 1,
						     "  %3d |  Attr | A %3d    "
						     "|   Known attrValue 0x%02X          "
						     "|       %sattrValue_0x%02X",
						     level, *codepage_attr, peek & 0x7f, Indent (level),
						     peek);
				off++;
			} else { /* attrStart */
				proto_tree_add_text (tree, tvb, off, 1,
						     "  %3d |  Attr | A %3d    "
						     "|   Known attrStart 0x%02X          "
						     "|   %sattrStart_0x%02X",
						     level, *codepage_attr, peek & 0x7f, Indent (level),
						     peek);
				off++;
			}
		}
	} /* End WHILE */
	DebugLog(("ATTR: level = %u, Return: len = %u (end of function body)\n",
		  level, off - offset));
	return (off - offset);
}