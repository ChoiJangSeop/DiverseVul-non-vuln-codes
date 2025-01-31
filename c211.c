static int sctp_process_inv_paramlength(const struct sctp_association *asoc,
					struct sctp_paramhdr *param,
					const struct sctp_chunk *chunk,
					struct sctp_chunk **errp)
{
	static const char error[] = "The following parameter had invalid length:";
	size_t		payload_len = WORD_ROUND(sizeof(error)) +
						sizeof(sctp_paramhdr_t);


	/* This is a fatal error.  Any accumulated non-fatal errors are
	 * not reported.
	 */
	if (*errp)
		sctp_chunk_free(*errp);

	/* Create an error chunk and fill it in with our payload. */
	*errp = sctp_make_op_error_space(asoc, chunk, payload_len);

	if (*errp) {
		sctp_init_cause(*errp, SCTP_ERROR_PROTO_VIOLATION,
				sizeof(error) + sizeof(sctp_paramhdr_t));
		sctp_addto_chunk(*errp, sizeof(error), error);
		sctp_addto_param(*errp, sizeof(sctp_paramhdr_t), param);
	}

	return 0;
}