static void io_ring_ctx_free(struct io_ring_ctx *ctx)
{
	io_finish_async(ctx);
	if (ctx->sqo_mm)
		mmdrop(ctx->sqo_mm);

	io_iopoll_reap_events(ctx);
	io_sqe_buffer_unregister(ctx);
	io_sqe_files_unregister(ctx);
	io_eventfd_unregister(ctx);

#if defined(CONFIG_UNIX)
	if (ctx->ring_sock) {
		ctx->ring_sock->file = NULL; /* so that iput() is called */
		sock_release(ctx->ring_sock);
	}
#endif

	io_mem_free(ctx->rings);
	io_mem_free(ctx->sq_sqes);

	percpu_ref_exit(&ctx->refs);
	if (ctx->account_mem)
		io_unaccount_mem(ctx->user,
				ring_pages(ctx->sq_entries, ctx->cq_entries));
	free_uid(ctx->user);
	kfree(ctx->completions);
	kmem_cache_free(req_cachep, ctx->fallback_req);
	kfree(ctx);
}