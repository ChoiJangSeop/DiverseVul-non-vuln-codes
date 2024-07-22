parse_wbxml_tag_defined (proto_tree *tree, tvbuff_t *tvb, guint32 offset,
			 guint32 str_tbl, guint8 *level, guint8 *codepage_stag, guint8 *codepage_attr,
			 const wbxml_decoding *map)
{
	guint32     tvb_len  = tvb_reported_length (tvb);
	guint32     off      = offset;
	guint32     len;
	guint       str_len;
	guint32     ent;
	guint32     idx;
	guint8      peek;
	guint32     tag_len;                     /* Length of the index (uintvar) from a LITERAL tag */
	guint8      tag_save_known      = 0;     /* Will contain peek & 0x3F (tag identity) */
	guint8      tag_new_known       = 0;     /* Will contain peek & 0x3F (tag identity) */
	const char *tag_save_literal;            /* Will contain the LITERAL tag identity */
	const char *tag_new_literal;             /* Will contain the LITERAL tag identity */
	guint8      parsing_tag_content = FALSE; /* Are we parsing content from a
					            tag with content: <x>Content</x>

					            The initial state is FALSE.
					            This state will trigger recursion. */
	tag_save_literal = NULL;                 /* Prevents compiler warning */

	DebugLog(("parse_wbxml_tag_defined (level = %u, offset = %u)\n", *level, offset));
	while (off < tvb_len) {
		peek = tvb_get_guint8 (tvb, off);
		DebugLog(("STAG: (top of while) level = %3u, peek = 0x%02X, off = %u, tvb_len = %u\n", *level, peek, off, tvb_len));
		if ((peek & 0x3F) < 4) switch (peek) { /* Global tokens in state = STAG
							  but not the LITERAL tokens */
		case 0x00: /* SWITCH_PAGE */
			*codepage_stag = tvb_get_guint8 (tvb, off+1);
			proto_tree_add_text (tree, tvb, off, 2,
					     "      | Tag   | T -->%3d "
					     "| SWITCH_PAGE (Tag code page)     "
					     "|",
					     *codepage_stag);
			off += 2;
			break;
		case 0x01: /* END: only possible for Tag with Content */
			if (tag_save_known) { /* Known TAG */
				proto_tree_add_text (tree, tvb, off, 1,
						     "  %3d | Tag   | T %3d    "
						     "| END (Known Tag 0x%02X)            "
						     "| %s</%s>",
						     *level, *codepage_stag,
						     tag_save_known, Indent (*level),
						     tag_save_literal); /* We already looked it up! */
			} else { /* Literal TAG */
				proto_tree_add_text (tree, tvb, off, 1,
						     "  %3d | Tag   | T %3d    "
						     "| END (Literal Tag)               "
						     "| %s</%s>",
						     *level, *codepage_stag, Indent (*level),
						     tag_save_literal ? tag_save_literal : "");
			}
			(*level)--;
			off++;
			/* Reset code page: not needed as return from recursion */
			DebugLog(("STAG: level = %u, Return: len = %u\n", *level, off - offset));
			return (off - offset);
		case 0x02: /* ENTITY */
			ent = tvb_get_guintvar (tvb, off+1, &len);
			proto_tree_add_text (tree, tvb, off, 1+len,
					     "  %3d | Tag   | T %3d    "
					     "| ENTITY                          "
					     "| %s'&#%u;'",
					     *level, *codepage_stag, Indent (*level), ent);
			off += 1+len;
			break;
		case 0x03: /* STR_I */
			len = tvb_strsize (tvb, off+1);
			proto_tree_add_text (tree, tvb, off, 1+len,
					     "  %3d | Tag   | T %3d    "
					     "| STR_I (Inline string)           "
					     "| %s\'%s\'",
					     *level, *codepage_stag, Indent(*level),
					     tvb_format_text (tvb, off+1, len-1));
			off += 1+len;
			break;
		case 0x40: /* EXT_I_0 */
		case 0x41: /* EXT_I_1 */
		case 0x42: /* EXT_I_2 */
			/* Extension tokens */
			len = tvb_strsize (tvb, off+1);
			proto_tree_add_text (tree, tvb, off, 1+len,
					     "  %3d | Tag   | T %3d    "
					     "| EXT_I_%1x    (Extension Token)    "
					     "| %s(%s: \'%s\')",
					     *level, *codepage_stag,
					     peek & 0x0f, Indent (*level),
					     map_token (map->global, 0, peek),
					     tvb_format_text (tvb, off+1, len-1));
			off += 1+len;
			break;
		case 0x43: /* PI */
			proto_tree_add_text (tree, tvb, off, 1,
					     "  %3d | Tag   | T %3d    "
					     "| PI (XML Processing Instruction) "
					     "| %s<?xml",
					     *level, *codepage_stag, Indent (*level));
			len = parse_wbxml_attribute_list_defined (tree, tvb, off,
								  str_tbl, *level, codepage_attr, map);
			/* Check that there is still room in packet */
			off += len;
			if (off >= tvb_len) {
				DebugLog(("STAG: level = %u, ThrowException: len = %u (short frame)\n", *level, off - offset));
				/*
				 * TODO - Do we need to free g_malloc()ed memory?
				 */
				THROW(ReportedBoundsError);
			}
			proto_tree_add_text (tree, tvb, off-1, 1,
					     "  %3d | Tag   | T %3d    "
					     "| END (PI)                        "
					     "| %s?>",
					     *level, *codepage_stag, Indent (*level));
			break;
		case 0x80: /* EXT_T_0 */
		case 0x81: /* EXT_T_1 */
		case 0x82: /* EXT_T_2 */
			/* Extension tokens */
			idx = tvb_get_guintvar (tvb, off+1, &len);
			{   char *s;
				if (map->ext_t[peek & 0x03])
					s = (map->ext_t[peek & 0x03])(tvb, idx, str_tbl);
				else
					s = wmem_strdup_printf(wmem_packet_scope(), "EXT_T_%1x (%s)", peek & 0x03,
							    map_token (map->global, 0, peek));
				proto_tree_add_text (tree, tvb, off, 1+len,
						     "  %3d | Tag   | T %3d    "
						     "| EXT_T_%1x    (Extension Token)    "
						     "| %s%s",
						     *level, *codepage_stag, peek & 0x0f, Indent (*level),
						     s);
			}
			off += 1+len;
			break;
		case 0x83: /* STR_T */
			idx = tvb_get_guintvar (tvb, off+1, &len);
			str_len = tvb_strsize (tvb, str_tbl+idx);
			proto_tree_add_text (tree, tvb, off, 1+len,
					     "  %3d | Tag   | T %3d    "
					     "| STR_T (Tableref string)         "
					     "| %s\'%s\'",
					     *level, *codepage_stag, Indent (*level),
					     tvb_format_text (tvb, str_tbl+idx, str_len-1));
			off += 1+len;
			break;
		case 0xC0: /* EXT_0 */
		case 0xC1: /* EXT_1 */
		case 0xC2: /* EXT_2 */
			/* Extension tokens */
			proto_tree_add_text (tree, tvb, off, 1,
					     "  %3d | Tag   | T %3d    "
					     "| EXT_%1x      (Extension Token)    "
					     "| %s(%s)",
					     *level, *codepage_stag, peek & 0x0f, Indent (*level),
					     map_token (map->global, 0, peek));
			off++;
			break;
		case 0xC3: /* OPAQUE - WBXML 1.1 and newer */
			if (tvb_get_guint8 (tvb, 0)) { /* WBXML 1.x (x > 0) */
				char *str;
				if (tag_save_known) { /* Knwon tag */
					if (map->opaque_binary_tag) {
						str = map->opaque_binary_tag(tvb, off + 1,
									     tag_save_known, *codepage_stag, &len);
					} else {
						str = default_opaque_binary_tag(tvb, off + 1,
										tag_save_known, *codepage_stag, &len);
					}
				} else { /* lITERAL tag */
					if (map->opaque_literal_tag) {
						str = map->opaque_literal_tag(tvb, off + 1,
									      tag_save_literal, *codepage_stag, &len);
					} else {
						str = default_opaque_literal_tag(tvb, off + 1,
										 tag_save_literal, *codepage_stag, &len);
					}
				}
				proto_tree_add_text (tree, tvb, off, 1 + len,
						     "  %3d | Tag   | T %3d    "
						     "| OPAQUE (Opaque data)            "
						     "| %s%s",
						     *level, *codepage_stag, Indent (*level), str);
				off += 1 + len;
			} else { /* WBXML 1.0 - RESERVED_2 token (invalid) */
				proto_tree_add_text (tree, tvb, off, 1,
						     "  %3d | Tag   | T %3d    "
						     "| RESERVED_2     (Invalid Token!) "
						     "| WBXML 1.0 parsing stops here.",
						     *level, *codepage_stag);
				/* Stop processing as it is impossible to parse now */
				off = tvb_len;
				DebugLog(("STAG: level = %u, Return: len = %u\n", *level, off - offset));
				return (off - offset);
			}
			break;

			/* No default clause, as all cases have been treated */
		} else { /* LITERAL or Known TAG */
			/* We must store the initial tag, and also retrieve the new tag.
			 * For efficiency reasons, we store the literal tag representation
			 * for known tags too, so we can easily close the tag without the
			 * need of a new lookup and avoiding storage of token codepage.
			 *
			 * There are 4 possibilities:
			 *
			 *  1. Known tag followed by a known tag
			 *  2. Known tag followed by a LITERAL tag
			 *  3. LITERAL tag followed by Known tag
			 *  4. LITERAL tag followed by LITERAL tag
			 */

			/* Store the new tag */
			tag_len = 0;
			if ((peek & 0x3F) == 4) { /* LITERAL */
				DebugLog(("STAG: LITERAL tag (peek = 0x%02X, off = %u) - TableRef follows!\n", peek, off));
				idx = tvb_get_guintvar (tvb, off+1, &tag_len);
				str_len = tvb_strsize (tvb, str_tbl+idx);
				tag_new_literal = (const gchar*)tvb_get_ptr (tvb, str_tbl+idx, str_len);
				tag_new_known = 0; /* invalidate known tag_new */
			} else { /* Known tag */
				tag_new_known = peek & 0x3F;
				tag_new_literal = map_token (map->tags, *codepage_stag,
							     tag_new_known);
				/* Stored looked up tag name string */
			}

			/* Parsing of TAG starts HERE */
			if (peek & 0x40) { /* Content present */
				/* Content follows
				 * [!] An explicit END token is expected in these cases!
				 * ==> Recursion possible if we encounter a tag with content;
				 *     recursion will return at the explicit END token.
				 */
				if (parsing_tag_content) { /* Recurse */
					DebugLog(("STAG: Tag in Tag - RECURSE! (off = %u)\n", off));
					/* Do not process the attribute list:
					 * recursion will take care of it */
					(*level)++;
					len = parse_wbxml_tag_defined (tree, tvb, off, str_tbl,
								       level, codepage_stag, codepage_attr, map);
					off += len;
				} else { /* Now we will have content to parse */
					/* Save the start tag so we can properly close it later. */
					if ((peek & 0x3F) == 4) { /* Literal tag */
						tag_save_literal = tag_new_literal;
						tag_save_known = 0;
					} else { /* Known tag */
						tag_save_known = tag_new_known;
						tag_save_literal = tag_new_literal;
						/* The last statement avoids needless lookups */
					}
					/* Process the attribute list if present */
					if (peek & 0x80) { /* Content and Attribute list present */
						if (tag_new_known) { /* Known tag */
							proto_tree_add_text (tree, tvb, off, 1,
									     "  %3d | Tag   | T %3d    "
									     "|   Known Tag 0x%02X           (AC) "
									     "| %s<%s",
									     *level, *codepage_stag, tag_new_known,
									     Indent (*level), tag_new_literal);
							/* Tag string already looked up earlier! */
							off++;
						} else { /* LITERAL tag */
							proto_tree_add_text (tree, tvb, off, 1,
									     "  %3d | Tag   | T %3d    "
									     "| LITERAL_AC (Literal tag)   (AC) "
									     "| %s<%s",
									     *level, *codepage_stag, Indent (*level), tag_new_literal);
							off += 1 + tag_len;
						}
						len = parse_wbxml_attribute_list_defined (tree, tvb,
											  off, str_tbl, *level, codepage_attr, map);
						/* Check that there is still room in packet */
						off += len;
						if (off >= tvb_len) {
							DebugLog(("STAG: level = %u, ThrowException: len = %u (short frame)\n",
								  *level, off - offset));
							/*
							 * TODO - Do we need to free g_malloc()ed memory?
							 */
							THROW(ReportedBoundsError);
						}
						proto_tree_add_text (tree, tvb, off-1, 1,
								     "  %3d | Tag   | T %3d    "
								     "| END (attribute list)            "
								     "| %s>",
								     *level, *codepage_stag, Indent (*level));
					} else { /* Content, no Attribute list */
						if (tag_new_known) { /* Known tag */
							proto_tree_add_text (tree, tvb, off, 1,
									     "  %3d | Tag   | T %3d    "
									     "|   Known Tag 0x%02X           (.C) "
									     "| %s<%s>",
									     *level, *codepage_stag, tag_new_known,
									     Indent (*level), tag_new_literal);
							/* Tag string already looked up earlier! */
							off++;
						} else { /* LITERAL tag */
							proto_tree_add_text (tree, tvb, off, 1,
									     "  %3d | Tag   | T %3d    "
									     "| LITERAL_C  (Literal Tag)   (.C) "
									     "| %s<%s>",
									     *level, *codepage_stag, Indent (*level),
									     tag_new_literal);
							off += 1 + tag_len;
						}
					}
					/* The data that follows in the parsing process
					 * represents content for the opening tag
					 * we've just processed in the lines above.
					 * Next time we encounter a tag with content: recurse
					 */
					parsing_tag_content = TRUE;
					DebugLog(("Tag in Tag - No recursion this time! (off = %u)\n", off));
				}
			} else { /* No Content */
				DebugLog(("<Tag/> in Tag - No recursion! (off = %u)\n", off));
				(*level)++;
				if (peek & 0x80) { /* No Content, Attribute list present */
					if (tag_new_known) { /* Known tag */
						proto_tree_add_text (tree, tvb, off, 1,
								     "  %3d | Tag   | T %3d    "
								     "|   Known Tag 0x%02X           (A.) "
								     "| %s<%s",
								     *level, *codepage_stag, tag_new_known,
								     Indent (*level), tag_new_literal);
						/* Tag string already looked up earlier! */
						off++;
						len = parse_wbxml_attribute_list_defined (tree, tvb,
											  off, str_tbl, *level, codepage_attr, map);
						/* Check that there is still room in packet */
						off += len;
						if (off > tvb_len) {
							DebugLog(("STAG: level = %u, ThrowException: len = %u (short frame)\n", *level, off - offset));
							/*
							 * TODO - Do we need to free g_malloc()ed memory?
							 */
							THROW(ReportedBoundsError);
						}
						proto_tree_add_text (tree, tvb, off-1, 1,
								     "  %3d | Tag   | T %3d    "
								     "| END (Known Tag)                 "
								     "| %s/>",
								     *level, *codepage_stag, Indent (*level));
					} else { /* LITERAL tag */
						proto_tree_add_text (tree, tvb, off, 1,
								     "  %3d | Tag   | T %3d    "
								     "| LITERAL_A  (Literal Tag)   (A.) "
								     "| %s<%s",
								     *level, *codepage_stag, Indent (*level), tag_new_literal);
						off += 1 + tag_len;
						len = parse_wbxml_attribute_list_defined (tree, tvb,
											  off, str_tbl, *level, codepage_attr, map);
						/* Check that there is still room in packet */
						off += len;
						if (off >= tvb_len) {
							DebugLog(("STAG: level = %u, ThrowException: len = %u (short frame)\n", *level, off - offset));
							/*
							 * TODO - Do we need to free g_malloc()ed memory?
							 */
							THROW(ReportedBoundsError);
						}
						proto_tree_add_text (tree, tvb, off-1, 1,
								     "  %3d | Tag   | T %3d    "
								     "| END (Literal Tag)               "
								     "| %s/>",
								     *level, *codepage_stag, Indent (*level));
					}
				} else { /* No Content, No Attribute list */
					if (tag_new_known) { /* Known tag */
						proto_tree_add_text (tree, tvb, off, 1,
								     "  %3d | Tag   | T %3d    "
								     "|   Known Tag 0x%02x           (..) "
								     "| %s<%s />",
								     *level, *codepage_stag, tag_new_known,
								     Indent (*level), tag_new_literal);
						/* Tag string already looked up earlier! */
						off++;
					} else { /* LITERAL tag */
						proto_tree_add_text (tree, tvb, off, 1,
								     "  %3d | Tag   | T %3d    "
								     "| LITERAL    (Literal Tag)   (..) "
								     "| %s<%s />",
								     *level, *codepage_stag, Indent (*level),
								     tag_new_literal);
						off += 1 + tag_len;
					}
				}
				(*level)--;
				/* TODO: Do I have to reset code page here? */
			}
		} /* if (tag & 0x3F) >= 5 */
	} /* while */
	DebugLog(("STAG: level = %u, Return: len = %u (end of function body)\n", *level, off - offset));
	return (off - offset);
}