idn2_strerror_name (int rc)
{
  switch (rc)
    {
    case IDN2_OK: return ERR2STR (IDN2_OK);
    case IDN2_MALLOC: return ERR2STR (IDN2_MALLOC);
    case IDN2_NO_CODESET:  return ERR2STR (IDN2_NO_NODESET);
    case IDN2_ICONV_FAIL:  return  ERR2STR (IDN2_ICONV_FAIL);
    case IDN2_ENCODING_ERROR: return ERR2STR (IDN2_ENCODING_ERROR);
    case IDN2_NFC: return ERR2STR (IDN2_NFC);
    case IDN2_PUNYCODE_BAD_INPUT: return ERR2STR (IDN2_PUNYCODE_BAD_INPUT);
    case IDN2_PUNYCODE_BIG_OUTPUT: return ERR2STR (IDN2_PUNYCODE_BIG_OUTPUT);
    case IDN2_PUNYCODE_OVERFLOW: return ERR2STR (IDN2_PUNYCODE_OVERFLOW);
    case IDN2_TOO_BIG_DOMAIN: return ERR2STR (IDN2_TOO_BIG_DOMAIN);
    case IDN2_TOO_BIG_LABEL: return ERR2STR (IDN2_TOO_BIG_LABEL);
    case IDN2_INVALID_ALABEL: return ERR2STR (IDN2_INVALID_ALABEL);
    case IDN2_UALABEL_MISMATCH: return ERR2STR (IDN2_UALABEL_MISMATCH);
    case IDN2_INVALID_FLAGS: return ERR2STR (IDN2_INVALID_FLAGS);
    case IDN2_NOT_NFC: return ERR2STR (IDN2_NOT_NFC);
    case IDN2_2HYPHEN: return ERR2STR (IDN2_2HYPHEN);
    case IDN2_HYPHEN_STARTEND: return ERR2STR (IDN2_HYPHEN_STARTEND);
    case IDN2_LEADING_COMBINING: return ERR2STR (IDN2_LEADING_COMBINING);
    case IDN2_DISALLOWED: return ERR2STR (IDN2_DISALLOWED);
    case IDN2_CONTEXTJ: return ERR2STR (IDN2_CONTEXTJ);
    case IDN2_CONTEXTJ_NO_RULE: return ERR2STR (IDN2_CONTEXTJ_NO_RULE);
    case IDN2_CONTEXTO: return ERR2STR (IDN2_CONTEXTO);
    case IDN2_CONTEXTO_NO_RULE: return ERR2STR (IDN2_CONTEXTO_NO_RULE);
    case IDN2_UNASSIGNED: return ERR2STR (IDN2_UNASSIGNED);
    case IDN2_BIDI: return ERR2STR (IDN2_BIDI);
    case IDN2_DOT_IN_LABEL: return ERR2STR (IDN2_DOT_IN_LABEL);
    case IDN2_INVALID_TRANSITIONAL: return ERR2STR (IDN2_INVALID_TRANSITIONAL);
    case IDN2_INVALID_NONTRANSITIONAL: return ERR2STR (IDN2_INVALID_NONTRANSITIONAL);
    default: return "IDN2_UNKNOWN";
    }
}