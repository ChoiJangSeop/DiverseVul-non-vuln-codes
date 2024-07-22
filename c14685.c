void jslGetTokenString(char *str, size_t len) {
  if (lex->tk == LEX_ID) {
    strcpy_s(str, len, "ID:");
    strncat(str, jslGetTokenValueAsString(), len);
  } else if (lex->tk == LEX_STR) {
    strcpy_s(str, len, "String:'");
    strncat(str, jslGetTokenValueAsString(), len);
    strncat(str, "'", len);
  } else
    jslTokenAsString(lex->tk, str, len);
}