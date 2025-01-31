sasl_process_packet(struct sasl_session *const restrict p, char *const restrict buf, const size_t len)
{
	struct sasl_output_buf outbuf = {
		.buf    = NULL,
		.len    = 0,
		.flags  = ASASL_OUTFLAG_NONE,
	};

	enum sasl_mechanism_result rc;
	bool have_responded = false;

	if (! p->mechptr && ! len)
	{
		// First piece of data in a session is the name of the SASL mechanism that will be used
		if (! (p->mechptr = sasl_mechanism_find(buf)))
		{
			(void) sasl_sts(p->uid, 'M', sasl_mechlist_string);
			return false;
		}

		(void) sasl_sourceinfo_recreate(p);

		if (p->mechptr->mech_start)
			rc = p->mechptr->mech_start(p, &outbuf);
		else
			rc = ASASL_MRESULT_CONTINUE;
	}
	else if (! p->mechptr)
	{
		(void) slog(LG_DEBUG, "%s: session has no mechanism?", MOWGLI_FUNC_NAME);
		return false;
	}
	else
	{
		rc = sasl_process_input(p, buf, len, &outbuf);
	}

	if (outbuf.buf && outbuf.len)
	{
		if (! sasl_process_output(p, &outbuf))
			return false;

		have_responded = true;
	}

	// Some progress has been made, reset timeout.
	p->flags &= ~ASASL_SFLAG_MARKED_FOR_DELETION;

	switch (rc)
	{
		case ASASL_MRESULT_CONTINUE:
		{
			if (! have_responded)
				/* We want more data from the client, but we haven't sent any of our own.
				 * Send an empty string to advance the session.    -- amdj
				 */
				(void) sasl_sts(p->uid, 'C', "+");

			return true;
		}

		case ASASL_MRESULT_SUCCESS:
		{
			struct user *const u = user_find(p->uid);
			struct myuser *const mu = sasl_user_can_login(p);

			if (! mu)
			{
				if (u)
					(void) notice(saslsvs->nick, u->nick, LOGIN_CANCELLED_STR);

				return false;
			}

			/* If the user is already on the network, attempt to log them in immediately.
			 * Otherwise, we will log them in on introduction of user to network
			 */
			if (u && ! sasl_handle_login(p, u, mu))
				return false;

			return sasl_session_success(p, mu, (u != NULL));
		}

		case ASASL_MRESULT_FAILURE:
		{
			if (*p->authceid)
			{
				/* If we reach this, they failed SASL auth, so if they were trying
				 * to authenticate as a specific user, run bad_password() on them.
				 */
				struct myuser *const mu = myuser_find_uid(p->authceid);

				if (! mu)
					return false;

				/* We might have more information to construct a more accurate sourceinfo now?
				 * TODO: Investigate whether this is necessary
				 */
				(void) sasl_sourceinfo_recreate(p);

				(void) logcommand(p->si, CMDLOG_LOGIN, "failed LOGIN (%s) to \2%s\2 (bad password)",
				                                       p->mechptr->name, entity(mu)->name);
				(void) bad_password(p->si, mu);
			}

			return false;
		}

		case ASASL_MRESULT_ERROR:
			return false;
	}

	/* This is only here to keep GCC happy -- Clang can see that the switch() handles all legal
	 * values of the enumeration, and so knows that this function will never get to this point;
	 * GCC is dumb, and warns that control reaches the end of this non-void function.    -- amdj
	 */
	return false;
}