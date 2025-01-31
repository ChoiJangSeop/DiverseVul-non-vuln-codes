
int read_escaped_char(
    yyscan_t yyscanner,
    uint8_t* escaped_char)
{
  char text[4] = {0, 0, 0, 0};

  text[0] = '\\';
  text[1] = RE_YY_INPUT(yyscanner);

  if (text[1] == EOF)
    return 0;

  if (text[1] == 'x')
  {
    text[2] = RE_YY_INPUT(yyscanner);

    if (text[2] == EOF)
      return 0;

    text[3] = RE_YY_INPUT(yyscanner);

    if (text[3] == EOF)
      return 0;
  }

  *escaped_char = escaped_char_value(text);

  return 1;