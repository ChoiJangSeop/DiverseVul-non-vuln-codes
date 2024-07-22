h2_end_headers(struct worker *wrk, struct h2_sess *h2,
    struct req *req, struct h2_req *r2)
{
	h2_error h2e;

	ASSERT_RXTHR(h2);
	assert(r2->state == H2_S_OPEN);
	h2e = h2h_decode_fini(h2);
	h2->new_req = NULL;
	if (h2e != NULL) {
		Lck_Lock(&h2->sess->mtx);
		VSLb(h2->vsl, SLT_Debug, "HPACK/FINI %s", h2e->name);
		Lck_Unlock(&h2->sess->mtx);
		assert(!WS_IsReserved(r2->req->ws));
		h2_del_req(wrk, r2);
		return (h2e);
	}
	VSLb_ts_req(req, "Req", req->t_req);

	// XXX: Smarter to do this already at HPACK time into tail end of
	// XXX: WS, then copy back once all headers received.
	// XXX: Have I mentioned H/2 Is hodge-podge ?
	http_CollectHdrSep(req->http, H_Cookie, "; ");	// rfc7540,l,3114,3120

	if (req->req_body_status == NULL) {
		if (!http_GetHdr(req->http, H_Content_Length, NULL))
			req->req_body_status = BS_EOF;
		else
			req->req_body_status = BS_LENGTH;
	} else {
		/* A HEADER frame contained END_STREAM */
		assert (req->req_body_status == BS_NONE);
		r2->state = H2_S_CLOS_REM;
		if (http_GetContentLength(req->http) > 0)
			return (H2CE_PROTOCOL_ERROR); //rfc7540,l,1838,1840
	}

	if (req->http->hd[HTTP_HDR_METHOD].b == NULL) {
		VSLb(h2->vsl, SLT_Debug, "Missing :method");
		return (H2SE_PROTOCOL_ERROR); //rfc7540,l,3087,3090
	}
	if (req->http->hd[HTTP_HDR_URL].b == NULL) {
		VSLb(h2->vsl, SLT_Debug, "Missing :path");
		return (H2SE_PROTOCOL_ERROR); //rfc7540,l,3087,3090
	}
	AN(req->http->hd[HTTP_HDR_PROTO].b);

	assert(req->req_step == R_STP_TRANSPORT);
	VCL_TaskEnter(req->privs);
	VCL_TaskEnter(req->top->privs);
	req->task->func = h2_do_req;
	req->task->priv = req;
	r2->scheduled = 1;
	if (Pool_Task(wrk->pool, req->task, TASK_QUEUE_STR) != 0) {
		r2->scheduled = 0;
		r2->state = H2_S_CLOSED;
		return (H2SE_REFUSED_STREAM); //rfc7540,l,3326,3329
	}
	return (0);
}