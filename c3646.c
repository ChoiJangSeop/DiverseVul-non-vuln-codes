static void php_snmp_error(zval *object, const char *docref, int type, const char *format, ...)
{
	va_list args;
	php_snmp_object *snmp_object = NULL;

	if (object) {
		snmp_object = Z_SNMP_P(object);
		if (type == PHP_SNMP_ERRNO_NOERROR) {
			memset(snmp_object->snmp_errstr, 0, sizeof(snmp_object->snmp_errstr));
		} else {
			va_start(args, format);
			vsnprintf(snmp_object->snmp_errstr, sizeof(snmp_object->snmp_errstr) - 1, format, args);
			va_end(args);
		}
		snmp_object->snmp_errno = type;
	}

	if (type == PHP_SNMP_ERRNO_NOERROR) {
		return;
	}

	if (object && (snmp_object->exceptions_enabled & type)) {
		zend_throw_exception_ex(php_snmp_exception_ce, type, snmp_object->snmp_errstr);
	} else {
		va_start(args, format);
		php_verror(docref, "", E_WARNING, format, args);
		va_end(args);
	}
}