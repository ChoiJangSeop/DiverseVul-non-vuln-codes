PHPAPI zend_string *php_escape_shell_arg(char *str)
{
	int x, y = 0, l = (int)strlen(str);
	zend_string *cmd;
	size_t estimate = (4 * l) + 3;


	cmd = zend_string_alloc(4 * l + 2, 0); /* worst case */

#ifdef PHP_WIN32
	ZSTR_VAL(cmd)[y++] = '"';
#else
	ZSTR_VAL(cmd)[y++] = '\'';
#endif

	for (x = 0; x < l; x++) {
		int mb_len = php_mblen(str + x, (l - x));

		/* skip non-valid multibyte characters */
		if (mb_len < 0) {
			continue;
		} else if (mb_len > 1) {
			memcpy(ZSTR_VAL(cmd) + y, str + x, mb_len);
			y += mb_len;
			x += mb_len - 1;
			continue;
		}

		switch (str[x]) {
#ifdef PHP_WIN32
		case '"':
		case '%':
		case '!':
			ZSTR_VAL(cmd)[y++] = ' ';
			break;
#else
		case '\'':
			ZSTR_VAL(cmd)[y++] = '\'';
			ZSTR_VAL(cmd)[y++] = '\\';
			ZSTR_VAL(cmd)[y++] = '\'';
#endif
			/* fall-through */
		default:
			ZSTR_VAL(cmd)[y++] = str[x];
		}
	}
#ifdef PHP_WIN32
	if (y > 0 && '\\' == ZSTR_VAL(cmd)[y - 1]) {
		int k = 0, n = y - 1;
		for (; n >= 0 && '\\' == ZSTR_VAL(cmd)[n]; n--, k++);
		if (k % 2) {
			ZSTR_VAL(cmd)[y++] = '\\';
		}
	}

	ZSTR_VAL(cmd)[y++] = '"';
#else
	ZSTR_VAL(cmd)[y++] = '\'';
#endif
	ZSTR_VAL(cmd)[y] = '\0';

	if ((estimate - y) > 4096) {
		/* realloc if the estimate was way overill
		 * Arbitrary cutoff point of 4096 */
		cmd = zend_string_truncate(cmd, y, 0);
	}
	ZSTR_LEN(cmd) = y;
	return cmd;
}