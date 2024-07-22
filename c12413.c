static void php_zip_get_from(INTERNAL_FUNCTION_PARAMETERS, int type) /* {{{ */
{
	struct zip *intern;
	zval *self = getThis();

	struct zip_stat sb;
	struct zip_file *zf;

	zend_long index = -1;
	zend_long flags = 0;
	zend_long len = 0;

	zend_string *filename;
	zend_string *buffer;

	int n = 0;

	if (!self) {
		RETURN_FALSE;
	}

	ZIP_FROM_OBJECT(intern, self);

	if (type == 1) {
		if (zend_parse_parameters(ZEND_NUM_ARGS(), "P|ll", &filename, &len, &flags) == FAILURE) {
			return;
		}
		PHP_ZIP_STAT_PATH(intern, ZSTR_VAL(filename), ZSTR_LEN(filename), flags, sb);
	} else {
		if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|ll", &index, &len, &flags) == FAILURE) {
			return;
		}
		PHP_ZIP_STAT_INDEX(intern, index, 0, sb);
	}

	if (sb.size < 1) {
		RETURN_EMPTY_STRING();
	}

	if (len < 1) {
		len = sb.size;
	}
	if (index >= 0) {
		zf = zip_fopen_index(intern, index, flags);
	} else {
		zf = zip_fopen(intern, ZSTR_VAL(filename), flags);
	}

	if (zf == NULL) {
		RETURN_FALSE;
	}

	buffer = zend_string_alloc(len, 0);
	n = zip_fread(zf, ZSTR_VAL(buffer), ZSTR_LEN(buffer));
	if (n < 1) {
		zend_string_free(buffer);
		RETURN_EMPTY_STRING();
	}

	zip_fclose(zf);
	ZSTR_VAL(buffer)[n] = '\0';
	ZSTR_LEN(buffer) = n;
	RETURN_NEW_STR(buffer);
}