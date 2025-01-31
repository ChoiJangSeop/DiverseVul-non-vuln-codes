dtls1_buffer_record(SSL *s, record_pqueue *queue, unsigned char *priority)
	{
	DTLS1_RECORD_DATA *rdata;
	pitem *item;

	/* Limit the size of the queue to prevent DOS attacks */
	if (pqueue_size(queue->q) >= 100)
		return 0;
		
	rdata = OPENSSL_malloc(sizeof(DTLS1_RECORD_DATA));
	item = pitem_new(priority, rdata);
	if (rdata == NULL || item == NULL)
		{
		if (rdata != NULL) OPENSSL_free(rdata);
		if (item != NULL) pitem_free(item);
		
		SSLerr(SSL_F_DTLS1_BUFFER_RECORD, ERR_R_INTERNAL_ERROR);
		return(0);
		}
	
	rdata->packet = s->packet;
	rdata->packet_length = s->packet_length;
	memcpy(&(rdata->rbuf), &(s->s3->rbuf), sizeof(SSL3_BUFFER));
	memcpy(&(rdata->rrec), &(s->s3->rrec), sizeof(SSL3_RECORD));

	item->data = rdata;

#ifndef OPENSSL_NO_SCTP
	/* Store bio_dgram_sctp_rcvinfo struct */
	if (BIO_dgram_is_sctp(SSL_get_rbio(s)) &&
	    (s->state == SSL3_ST_SR_FINISHED_A || s->state == SSL3_ST_CR_FINISHED_A)) {
		BIO_ctrl(SSL_get_rbio(s), BIO_CTRL_DGRAM_SCTP_GET_RCVINFO, sizeof(rdata->recordinfo), &rdata->recordinfo);
	}
#endif

	s->packet = NULL;
	s->packet_length = 0;
	memset(&(s->s3->rbuf), 0, sizeof(SSL3_BUFFER));
	memset(&(s->s3->rrec), 0, sizeof(SSL3_RECORD));
	
	if (!ssl3_setup_buffers(s))
		{
		SSLerr(SSL_F_DTLS1_BUFFER_RECORD, ERR_R_INTERNAL_ERROR);
		OPENSSL_free(rdata);
		pitem_free(item);
		return(0);
		}

	/* insert should not fail, since duplicates are dropped */
	if (pqueue_insert(queue->q, item) == NULL)
		{
		SSLerr(SSL_F_DTLS1_BUFFER_RECORD, ERR_R_INTERNAL_ERROR);
		OPENSSL_free(rdata);
		pitem_free(item);
		return(0);
		}

	return(1);
	}