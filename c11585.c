ins_compl_add_infercase(
    char_u	*str_arg,
    int		len,
    int		icase,
    char_u	*fname,
    int		dir,
    int		cont_s_ipos)  // next ^X<> will set initial_pos
{
    char_u	*str = str_arg;
    char_u	*p;
    int		actual_len;		// Take multi-byte characters
    int		actual_compl_length;	// into account.
    int		min_len;
    int		flags = 0;

    if (p_ic && curbuf->b_p_inf && len > 0)
    {
	// Infer case of completed part.

	// Find actual length of completion.
	if (has_mbyte)
	{
	    p = str;
	    actual_len = 0;
	    while (*p != NUL)
	    {
		MB_PTR_ADV(p);
		++actual_len;
	    }
	}
	else
	    actual_len = len;

	// Find actual length of original text.
	if (has_mbyte)
	{
	    p = compl_orig_text;
	    actual_compl_length = 0;
	    while (*p != NUL)
	    {
		MB_PTR_ADV(p);
		++actual_compl_length;
	    }
	}
	else
	    actual_compl_length = compl_length;

	// "actual_len" may be smaller than "actual_compl_length" when using
	// thesaurus, only use the minimum when comparing.
	min_len = actual_len < actual_compl_length
					   ? actual_len : actual_compl_length;

	str = ins_compl_infercase_gettext(str, actual_len, actual_compl_length,
								min_len);
    }
    if (cont_s_ipos)
	flags |= CP_CONT_S_IPOS;
    if (icase)
	flags |= CP_ICASE;

    return ins_compl_add(str, len, fname, NULL, NULL, dir, flags, FALSE);
}