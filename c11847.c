ex_endtry(exarg_T *eap)
{
    int		idx;
    int		skip;
    int		rethrow = FALSE;
    int		pending = CSTP_NONE;
    void	*rettv = NULL;
    cstack_T	*cstack = eap->cstack;

    if (cmdmod_error(FALSE))
	return;

    if (cstack->cs_trylevel <= 0 || cstack->cs_idx < 0)
	eap->errmsg = _(e_endtry_without_try);
    else
    {
	/*
	 * Don't do something after an error, interrupt or throw in the try
	 * block, catch clause, or finally clause preceding this ":endtry" or
	 * when an error or interrupt occurred after a ":continue", ":break",
	 * ":return", or ":finish" in a try block or catch clause preceding this
	 * ":endtry" or when the try block never got active (because of an
	 * inactive surrounding conditional or after an error or interrupt or
	 * throw) or when there is a surrounding conditional and it has been
	 * made inactive by a ":continue", ":break", ":return", or ":finish" in
	 * the finally clause.  The latter case need not be tested since then
	 * anything pending has already been discarded. */
	skip = did_emsg || got_int || did_throw
			     || !(cstack->cs_flags[cstack->cs_idx] & CSF_TRUE);

	if (!(cstack->cs_flags[cstack->cs_idx] & CSF_TRY))
	{
	    eap->errmsg = get_end_emsg(cstack);

	    // Find the matching ":try" and report what's missing.
	    idx = cstack->cs_idx;
	    do
		--idx;
	    while (idx > 0 && !(cstack->cs_flags[idx] & CSF_TRY));
	    rewind_conditionals(cstack, idx, CSF_WHILE | CSF_FOR,
						       &cstack->cs_looplevel);
	    skip = TRUE;

	    /*
	     * If an exception is being thrown, discard it to prevent it from
	     * being rethrown at the end of this function.  It would be
	     * discarded by the error message, anyway.  Resets did_throw.
	     * This does not affect the script termination due to the error
	     * since "trylevel" is decremented after emsg() has been called.
	     */
	    if (did_throw)
		discard_current_exception();

	    // report eap->errmsg, also when there already was an error
	    did_emsg = FALSE;
	}
	else
	{
	    idx = cstack->cs_idx;

	    // Check the flags only when not in a skipped block.
	    if (!skip && in_vim9script()
		     && (cstack->cs_flags[idx] & (CSF_CATCH|CSF_FINALLY)) == 0)
	    {
		// try/endtry without any catch or finally: give an error and
		// continue.
		eap->errmsg = _(e_missing_catch_or_finally);
	    }

	    /*
	     * If we stopped with the exception currently being thrown at this
	     * try conditional since we didn't know that it doesn't have
	     * a finally clause, we need to rethrow it after closing the try
	     * conditional.
	     */
	    if (did_throw && (cstack->cs_flags[idx] & CSF_TRUE)
		    && !(cstack->cs_flags[idx] & CSF_FINALLY))
		rethrow = TRUE;
	}

	// If there was no finally clause, show the user when debugging or
	// a breakpoint was encountered that the end of the try conditional has
	// been reached: display the debug prompt (if not already done).  Do
	// this on normal control flow or when an exception was thrown, but not
	// on an interrupt or error not converted to an exception or when
	// a ":break", ":continue", ":return", or ":finish" is pending.  These
	// actions are carried out immediately.
	if ((rethrow || (!skip
			&& !(cstack->cs_flags[idx] & CSF_FINALLY)
			&& !cstack->cs_pending[idx]))
		&& dbg_check_skipped(eap))
	{
	    // Handle a ">quit" debug command as if an interrupt had occurred
	    // before the ":endtry".  That is, throw an interrupt exception and
	    // set "skip" and "rethrow".
	    if (got_int)
	    {
		skip = TRUE;
		(void)do_intthrow(cstack);
		// The do_intthrow() call may have reset did_throw or
		// cstack->cs_pending[idx].
		rethrow = FALSE;
		if (did_throw && !(cstack->cs_flags[idx] & CSF_FINALLY))
		    rethrow = TRUE;
	    }
	}

	/*
	 * If a ":return" is pending, we need to resume it after closing the
	 * try conditional; remember the return value.  If there was a finally
	 * clause making an exception pending, we need to rethrow it.  Make it
	 * the exception currently being thrown.
	 */
	if (!skip)
	{
	    pending = cstack->cs_pending[idx];
	    cstack->cs_pending[idx] = CSTP_NONE;
	    if (pending == CSTP_RETURN)
		rettv = cstack->cs_rettv[idx];
	    else if (pending & CSTP_THROW)
		current_exception = cstack->cs_exception[idx];
	}

	/*
	 * Discard anything pending on an error, interrupt, or throw in the
	 * finally clause.  If there was no ":finally", discard a pending
	 * ":continue", ":break", ":return", or ":finish" if an error or
	 * interrupt occurred afterwards, but before the ":endtry" was reached.
	 * If an exception was caught by the last of the catch clauses and there
	 * was no finally clause, finish the exception now.  This happens also
	 * after errors except when this ":endtry" is not within a ":try".
	 * Restore "emsg_silent" if it has been reset by this try conditional.
	 */
	(void)cleanup_conditionals(cstack, CSF_TRY | CSF_SILENT, TRUE);

	if (cstack->cs_idx >= 0
			       && (cstack->cs_flags[cstack->cs_idx] & CSF_TRY))
	    leave_block(cstack);
	--cstack->cs_trylevel;

	if (!skip)
	{
	    report_resume_pending(pending,
		    (pending == CSTP_RETURN) ? rettv :
		    (pending & CSTP_THROW) ? (void *)current_exception : NULL);
	    switch (pending)
	    {
		case CSTP_NONE:
		    break;

		// Reactivate a pending ":continue", ":break", ":return",
		// ":finish" from the try block or a catch clause of this try
		// conditional.  This is skipped, if there was an error in an
		// (unskipped) conditional command or an interrupt afterwards
		// or if the finally clause is present and executed a new error,
		// interrupt, throw, ":continue", ":break", ":return", or
		// ":finish".
		case CSTP_CONTINUE:
		    ex_continue(eap);
		    break;
		case CSTP_BREAK:
		    ex_break(eap);
		    break;
		case CSTP_RETURN:
		    do_return(eap, FALSE, FALSE, rettv);
		    break;
		case CSTP_FINISH:
		    do_finish(eap, FALSE);
		    break;

		// When the finally clause was entered due to an error,
		// interrupt or throw (as opposed to a ":continue", ":break",
		// ":return", or ":finish"), restore the pending values of
		// did_emsg, got_int, and did_throw.  This is skipped, if there
		// was a new error, interrupt, throw, ":continue", ":break",
		// ":return", or ":finish".  in the finally clause.
		default:
		    if (pending & CSTP_ERROR)
			did_emsg = TRUE;
		    if (pending & CSTP_INTERRUPT)
			got_int = TRUE;
		    if (pending & CSTP_THROW)
			rethrow = TRUE;
		    break;
	    }
	}

	if (rethrow)
	    // Rethrow the current exception (within this cstack).
	    do_throw(cstack);
    }
}