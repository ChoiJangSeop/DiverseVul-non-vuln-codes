PHP_FUNCTION(stream_resolve_include_path)
{
	char *filename, *resolved_path;
	int filename_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
		return;
	}

	resolved_path = zend_resolve_path(filename, filename_len TSRMLS_CC);

	if (resolved_path) {
		RETURN_STRING(resolved_path, 0);
	}
	RETURN_FALSE;
}