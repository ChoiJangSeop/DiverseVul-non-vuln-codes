qedi_dbg_info(struct qedi_dbg_ctx *qedi, const char *func, u32 line,
	      u32 level, const char *fmt, ...)
{
	va_list va;
	struct va_format vaf;
	char nfunc[32];

	memset(nfunc, 0, sizeof(nfunc));
	memcpy(nfunc, func, sizeof(nfunc) - 1);

	va_start(va, fmt);

	vaf.fmt = fmt;
	vaf.va = &va;

	if (!(qedi_dbg_log & level))
		goto ret;

	if (likely(qedi) && likely(qedi->pdev))
		pr_info("[%s]:[%s:%d]:%d: %pV", dev_name(&qedi->pdev->dev),
			nfunc, line, qedi->host_no, &vaf);
	else
		pr_info("[0000:00:00.0]:[%s:%d]: %pV", nfunc, line, &vaf);

ret:
	va_end(va);
}