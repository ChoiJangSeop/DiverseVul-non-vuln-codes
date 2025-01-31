PS_SERIALIZER_DECODE_FUNC(php_serialize) /* {{{ */
{
	const char *endptr = val + vallen;
	zval *session_vars;
	php_unserialize_data_t var_hash;

	PHP_VAR_UNSERIALIZE_INIT(var_hash);
	ALLOC_INIT_ZVAL(session_vars);
	if (php_var_unserialize(&session_vars, &val, endptr, &var_hash TSRMLS_CC)) {
		var_push_dtor(&var_hash, &session_vars);
	}
	
	PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
	if (PS(http_session_vars)) {
		zval_ptr_dtor(&PS(http_session_vars));
	}
	if (Z_TYPE_P(session_vars) == IS_NULL) {
		array_init(session_vars);
	}
	PS(http_session_vars) = session_vars;
	ZEND_SET_GLOBAL_VAR_WITH_LENGTH("_SESSION", sizeof("_SESSION"), PS(http_session_vars), Z_REFCOUNT_P(PS(http_session_vars)) + 1, 1);
	return SUCCESS;
}