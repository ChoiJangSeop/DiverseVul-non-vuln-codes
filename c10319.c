static int vvalue_tvb_vector_internal(tvbuff_t *tvb, int offset, struct vt_vector *val, struct vtype_data *type, guint num)
{
	const int offset_in = offset;
	const gboolean varsize = (type->size == -1);
	const guint elsize = varsize ? (guint)sizeof(struct data_blob) : (guint)type->size;
	guint8 *data;
	int len;
	guint i;

	/*
	 * Make sure we actually *have* the data we're going to fetch
	 * here, before making a possibly-doomed attempt to allocate
	 * memory for it.
	 *
	 * First, check for an overflow.
	 */
	if ((guint64)elsize * (guint64)num > G_MAXUINT) {
		/*
		 * We never have more than G_MAXUINT bytes in a tvbuff,
		 * so this will *definitely* fail.
		 */
		THROW(ReportedBoundsError);
	}

	/*
	 * No overflow; now make sure we at least have that data.
	 */
	tvb_ensure_bytes_exist(tvb, offset, elsize * num);

	/*
	 * OK, it exists; allocate a buffer into which to fetch it.
	 */
	data = (guint8*)wmem_alloc(wmem_packet_scope(), elsize * num);

	val->len = num;
	val->u.vt_ui1 = data;
	DISSECTOR_ASSERT((void*)&val->u == ((void*)&val->u.vt_ui1));

	for (i=0; i<num; i++) {
		DISSECTOR_ASSERT_HINT(type->tvb_get != 0,
				      "type that we don't know yet how to handle, please submit a bug with trace");
		len = type->tvb_get(tvb, offset, data);
		data += elsize;
		offset += len;
		if (varsize && (offset % 4) ) { /* at begin or end of loop ??? */
			int padding = 4 - (offset % 4);
			offset += padding;
		}
	}
	return offset - offset_in;
}