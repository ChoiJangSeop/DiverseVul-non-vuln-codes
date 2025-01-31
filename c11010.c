may_get_cmd_block(exarg_T *eap, char_u *p, char_u **tofree, int *flags)
{
    char_u *retp = p;

    if (*p == '{' && ends_excmd2(eap->arg, skipwhite(p + 1))
						       && eap->getline != NULL)
    {
	garray_T    ga;
	char_u	    *line = NULL;

	ga_init2(&ga, sizeof(char_u *), 10);
	if (ga_add_string(&ga, p) == FAIL)
	    return retp;

	// If the argument ends in "}" it must have been concatenated already
	// for ISN_EXEC.
	if (p[STRLEN(p) - 1] != '}')
	    // Read lines between '{' and '}'.  Does not support nesting or
	    // here-doc constructs.
	    for (;;)
	    {
		vim_free(line);
		if ((line = eap->getline(':', eap->cookie,
					   0, GETLINE_CONCAT_CONTBAR)) == NULL)
		{
		    emsg(_(e_missing_rcurly));
		    break;
		}
		if (ga_add_string(&ga, line) == FAIL)
		    break;
		if (*skipwhite(line) == '}')
		    break;
	    }
	vim_free(line);
	retp = *tofree = ga_concat_strings(&ga, "\n");
	ga_clear_strings(&ga);
	*flags |= UC_VIM9;
    }
    return retp;
}