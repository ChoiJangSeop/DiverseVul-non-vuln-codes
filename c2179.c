int sctp_verify_asconf(const struct sctp_association *asoc,
		       struct sctp_paramhdr *param_hdr, void *chunk_end,
		       struct sctp_paramhdr **errp) {
	sctp_addip_param_t *asconf_param;
	union sctp_params param;
	int length, plen;

	param.v = (sctp_paramhdr_t *) param_hdr;
	while (param.v <= chunk_end - sizeof(sctp_paramhdr_t)) {
		length = ntohs(param.p->length);
		*errp = param.p;

		if (param.v > chunk_end - length ||
		    length < sizeof(sctp_paramhdr_t))
			return 0;

		switch (param.p->type) {
		case SCTP_PARAM_ADD_IP:
		case SCTP_PARAM_DEL_IP:
		case SCTP_PARAM_SET_PRIMARY:
			asconf_param = (sctp_addip_param_t *)param.v;
			plen = ntohs(asconf_param->param_hdr.length);
			if (plen < sizeof(sctp_addip_param_t) +
			    sizeof(sctp_paramhdr_t))
				return 0;
			break;
		case SCTP_PARAM_SUCCESS_REPORT:
		case SCTP_PARAM_ADAPTATION_LAYER_IND:
			if (length != sizeof(sctp_addip_param_t))
				return 0;

			break;
		default:
			break;
		}

		param.v += WORD_ROUND(length);
	}

	if (param.v != chunk_end)
		return 0;

	return 1;
}