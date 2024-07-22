static void gf_dump_vrml_dyn_field(GF_SceneDumper *sdump, GF_Node *node, GF_FieldInfo field, Bool has_sublist)
{
	u32 i, sf_type;
	void *slot_ptr;

	if (gf_sg_vrml_is_sf_field(field.fieldType)) {
		DUMP_IND(sdump);
		if (sdump->XMLDump) {
			if (sdump->X3DDump) {
				gf_fprintf(sdump->trace, "<field name=\"%s\" type=\"%s\" accessType=\"%s\"",
				        field.name, gf_sg_vrml_get_field_type_name(field.fieldType), gf_sg_vrml_get_event_type_name(field.eventType, 1));
			} else {
				gf_fprintf(sdump->trace, "<field name=\"%s\" type=\"%s\" vrml97Hint=\"%s\"",
				        field.name, GetXMTFieldTypeName(field.fieldType), gf_sg_vrml_get_event_type_name(field.eventType, 0));
			}

			if ((field.eventType == GF_SG_EVENT_FIELD) || (field.eventType == GF_SG_EVENT_EXPOSED_FIELD)) {
				if (field.fieldType == GF_SG_VRML_SFNODE) {
					if (!sdump->X3DDump) {
						gf_fprintf(sdump->trace, ">\n");
						sdump->indent++;
						gf_fprintf(sdump->trace, "<node>");
						gf_dump_vrml_node(sdump, field.far_ptr ? *(GF_Node **)field.far_ptr : NULL, 0, NULL);
						gf_fprintf(sdump->trace, "</node>");
						sdump->indent--;
						if (!has_sublist)
							gf_fprintf(sdump->trace, "</field>\n");
					} else {
						if (field.far_ptr) {
							gf_fprintf(sdump->trace, ">\n");
							gf_dump_vrml_node(sdump, *(GF_Node **)field.far_ptr, 0, NULL);
							gf_fprintf(sdump->trace, "</field>\n");
						} else {
							gf_fprintf(sdump->trace, "/>\n");
						}
					}
					DUMP_IND(sdump);
				} else {
					if (sdump->X3DDump) {
						gf_fprintf(sdump->trace, " value=\"");
					} else {
						gf_fprintf(sdump->trace, " %s=\"", GetXMTFieldTypeValueName(field.fieldType));
					}
					if (field.far_ptr)
						gf_dump_vrml_sffield(sdump, field.fieldType, field.far_ptr, 0, node);
					if (has_sublist)
						gf_fprintf(sdump->trace, "\">\n");
					else
						gf_fprintf(sdump->trace, "\"/>\n");
				}
			} else {
				gf_fprintf(sdump->trace, "/>\n");
			}
		} else {
			gf_fprintf(sdump->trace, "%s %s %s", gf_sg_vrml_get_event_type_name(field.eventType, sdump->X3DDump), gf_sg_vrml_get_field_type_name(field.fieldType), field.name);
			if ((field.eventType==GF_SG_EVENT_FIELD) || (field.eventType==GF_SG_EVENT_EXPOSED_FIELD)) {
				gf_fprintf(sdump->trace, " ");
				if (field.fieldType == GF_SG_VRML_SFNODE) {
					gf_dump_vrml_node(sdump, field.far_ptr ? *(GF_Node **)field.far_ptr : NULL, 0, NULL);
				} else if (field.far_ptr) {
					gf_dump_vrml_simple_field(sdump, field, node);
				}
			}
			gf_fprintf(sdump->trace, "\n");
		}
	} else if (field.far_ptr) {
		GenMFField *mffield = (GenMFField *) field.far_ptr;
		sf_type = gf_sg_vrml_get_sf_type(field.fieldType);

		DUMP_IND(sdump);
		if (!sdump->XMLDump) {
			gf_fprintf(sdump->trace, "%s %s %s", gf_sg_vrml_get_event_type_name(field.eventType, sdump->X3DDump), gf_sg_vrml_get_field_type_name(field.fieldType), field.name);
			if ((field.eventType==GF_SG_EVENT_FIELD) || (field.eventType==GF_SG_EVENT_EXPOSED_FIELD)) {
				gf_fprintf(sdump->trace, " [");

				if (sf_type == GF_SG_VRML_SFNODE) {
					GF_ChildNodeItem *l = *(GF_ChildNodeItem **)field.far_ptr;
					gf_fprintf(sdump->trace, "\n");
					sdump->indent++;
					while (l) {
						gf_dump_vrml_node(sdump, l->node, 1, NULL);
						l = l->next;
					}
					sdump->indent--;
					DUMP_IND(sdump);
				} else {
					for (i=0; i<mffield->count; i++) {
						if (i) gf_fprintf(sdump->trace, " ");
						if (field.fieldType != GF_SG_VRML_MFNODE) {
							gf_sg_vrml_mf_get_item(field.far_ptr, field.fieldType, &slot_ptr, i);
							gf_dump_vrml_sffield(sdump, sf_type, slot_ptr, (mffield->count>1) ? 1 : 0, node);
						}
					}
				}
				gf_fprintf(sdump->trace, "]");
			}
			gf_fprintf(sdump->trace, "\n");
		} else {
			if (sdump->X3DDump) {
				gf_fprintf(sdump->trace, "<field name=\"%s\" type=\"%s\" accessType=\"%s\"",
				        field.name, gf_sg_vrml_get_field_type_name(field.fieldType), gf_sg_vrml_get_event_type_name(field.eventType, 1));
			} else {
				gf_fprintf(sdump->trace, "<field name=\"%s\" type=\"%s\" vrml97Hint=\"%s\"",
				        field.name, GetXMTFieldTypeName(field.fieldType), gf_sg_vrml_get_event_type_name(field.eventType, 0));
			}

			if ((field.eventType==GF_SG_EVENT_FIELD) || (field.eventType==GF_SG_EVENT_EXPOSED_FIELD)) {
				if (sf_type == GF_SG_VRML_SFNODE) {
					GF_ChildNodeItem *list = *(GF_ChildNodeItem **)field.far_ptr;
					gf_fprintf(sdump->trace, ">\n");
					sdump->indent++;
					if (!sdump->X3DDump) gf_fprintf(sdump->trace, "<nodes>");
					while (list) {
						gf_dump_vrml_node(sdump, list->node, 1, NULL);
						list = list->next;
					}
					if (!sdump->X3DDump) gf_fprintf(sdump->trace, "</nodes>");
					sdump->indent++;
					DUMP_IND(sdump);
					if (!has_sublist)
						gf_fprintf(sdump->trace, "</field>\n");
				} else {
					if (sdump->X3DDump) {
						gf_fprintf(sdump->trace, " value=\"");
					} else {
						gf_fprintf(sdump->trace, " %s=\"", GetXMTFieldTypeValueName(field.fieldType));
					}
					for (i=0; i<mffield->count; i++) {
						if (i) gf_fprintf(sdump->trace, " ");
						if (field.fieldType != GF_SG_VRML_MFNODE) {
							gf_sg_vrml_mf_get_item(field.far_ptr, field.fieldType, &slot_ptr, i);
							gf_dump_vrml_sffield(sdump, sf_type, slot_ptr, (mffield->count>1) ? 1 : 0, node);
						}
					}
					if (has_sublist)
						gf_fprintf(sdump->trace, "\">\n");
					else
						gf_fprintf(sdump->trace, "\"/>\n");
				}
			} else {
				gf_fprintf(sdump->trace, "/>\n");
			}
		}
	}
}