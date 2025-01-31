void php_register_signal_constants(INIT_FUNC_ARGS)
{

	/* Wait Constants */
#ifdef WNOHANG
	REGISTER_LONG_CONSTANT("WNOHANG",  (long) WNOHANG, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef WUNTRACED
	REGISTER_LONG_CONSTANT("WUNTRACED",  (long) WUNTRACED, CONST_CS | CONST_PERSISTENT);
#endif

	/* Signal Constants */
	REGISTER_LONG_CONSTANT("SIG_IGN",  (long) SIG_IGN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIG_DFL",  (long) SIG_DFL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIG_ERR",  (long) SIG_ERR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGHUP",   (long) SIGHUP,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGINT",   (long) SIGINT,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGQUIT",  (long) SIGQUIT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGILL",   (long) SIGILL,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGTRAP",  (long) SIGTRAP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGABRT",  (long) SIGABRT, CONST_CS | CONST_PERSISTENT);
#ifdef SIGIOT
	REGISTER_LONG_CONSTANT("SIGIOT",   (long) SIGIOT,  CONST_CS | CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SIGBUS",   (long) SIGBUS,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGFPE",   (long) SIGFPE,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGKILL",  (long) SIGKILL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGUSR1",  (long) SIGUSR1, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGSEGV",  (long) SIGSEGV, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGUSR2",  (long) SIGUSR2, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGPIPE",  (long) SIGPIPE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGALRM",  (long) SIGALRM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGTERM",  (long) SIGTERM, CONST_CS | CONST_PERSISTENT);
#ifdef SIGSTKFLT
	REGISTER_LONG_CONSTANT("SIGSTKFLT",(long) SIGSTKFLT, CONST_CS | CONST_PERSISTENT);
#endif 
#ifdef SIGCLD
	REGISTER_LONG_CONSTANT("SIGCLD",   (long) SIGCLD, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef SIGCHLD
	REGISTER_LONG_CONSTANT("SIGCHLD",  (long) SIGCHLD, CONST_CS | CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SIGCONT",  (long) SIGCONT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGSTOP",  (long) SIGSTOP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGTSTP",  (long) SIGTSTP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGTTIN",  (long) SIGTTIN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGTTOU",  (long) SIGTTOU, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGURG",   (long) SIGURG , CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGXCPU",  (long) SIGXCPU, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGXFSZ",  (long) SIGXFSZ, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGVTALRM",(long) SIGVTALRM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGPROF",  (long) SIGPROF, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGWINCH", (long) SIGWINCH, CONST_CS | CONST_PERSISTENT);
#ifdef SIGPOLL
	REGISTER_LONG_CONSTANT("SIGPOLL",  (long) SIGPOLL, CONST_CS | CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SIGIO",    (long) SIGIO, CONST_CS | CONST_PERSISTENT);
#ifdef SIGPWR
	REGISTER_LONG_CONSTANT("SIGPWR",   (long) SIGPWR, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef SIGSYS
	REGISTER_LONG_CONSTANT("SIGSYS",   (long) SIGSYS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGBABY",  (long) SIGSYS, CONST_CS | CONST_PERSISTENT);
#endif

#if HAVE_GETPRIORITY || HAVE_SETPRIORITY
	REGISTER_LONG_CONSTANT("PRIO_PGRP", PRIO_PGRP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PRIO_USER", PRIO_USER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PRIO_PROCESS", PRIO_PROCESS, CONST_CS | CONST_PERSISTENT);
#endif

	/* {{{ "how" argument for sigprocmask */
#ifdef HAVE_SIGPROCMASK
	REGISTER_LONG_CONSTANT("SIG_BLOCK",   SIG_BLOCK, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIG_UNBLOCK", SIG_UNBLOCK, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIG_SETMASK", SIG_SETMASK, CONST_CS | CONST_PERSISTENT);
#endif
	/* }}} */

	/* {{{ si_code */
#if HAVE_SIGWAITINFO && HAVE_SIGTIMEDWAIT
	REGISTER_LONG_CONSTANT("SI_USER",    SI_USER,    CONST_CS | CONST_PERSISTENT);
#ifdef SI_NOINFO
	REGISTER_LONG_CONSTANT("SI_NOINFO",  SI_NOINFO,  CONST_CS | CONST_PERSISTENT);
#endif
#ifdef SI_KERNEL
	REGISTER_LONG_CONSTANT("SI_KERNEL",  SI_KERNEL,  CONST_CS | CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SI_QUEUE",   SI_QUEUE,   CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SI_TIMER",   SI_TIMER,   CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SI_MESGQ",   SI_MESGQ,   CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SI_ASYNCIO", SI_ASYNCIO, CONST_CS | CONST_PERSISTENT);
#ifdef SI_SIGIO
	REGISTER_LONG_CONSTANT("SI_SIGIO",   SI_SIGIO,   CONST_CS | CONST_PERSISTENT);
#endif
#ifdef SI_TKILL
	REGISTER_LONG_CONSTANT("SI_TKILL",   SI_TKILL,   CONST_CS | CONST_PERSISTENT);
#endif

	/* si_code for SIGCHILD */
#ifdef CLD_EXITED
	REGISTER_LONG_CONSTANT("CLD_EXITED",    CLD_EXITED,    CONST_CS | CONST_PERSISTENT);
#endif
#ifdef CLD_KILLED
	REGISTER_LONG_CONSTANT("CLD_KILLED",    CLD_KILLED,    CONST_CS | CONST_PERSISTENT);
#endif
#ifdef CLD_DUMPED
	REGISTER_LONG_CONSTANT("CLD_DUMPED",    CLD_DUMPED,    CONST_CS | CONST_PERSISTENT);
#endif
#ifdef CLD_TRAPPED
	REGISTER_LONG_CONSTANT("CLD_TRAPPED",   CLD_TRAPPED,   CONST_CS | CONST_PERSISTENT);
#endif
#ifdef CLD_STOPPED
	REGISTER_LONG_CONSTANT("CLD_STOPPED",   CLD_STOPPED,   CONST_CS | CONST_PERSISTENT);
#endif
#ifdef CLD_CONTINUED
	REGISTER_LONG_CONSTANT("CLD_CONTINUED", CLD_CONTINUED, CONST_CS | CONST_PERSISTENT);
#endif

	/* si_code for SIGTRAP */
#ifdef TRAP_BRKPT
	REGISTER_LONG_CONSTANT("TRAP_BRKPT", TRAP_BRKPT, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef TRAP_TRACE
	REGISTER_LONG_CONSTANT("TRAP_TRACE", TRAP_TRACE, CONST_CS | CONST_PERSISTENT);
#endif

	/* si_code for SIGPOLL */
#ifdef POLL_IN
	REGISTER_LONG_CONSTANT("POLL_IN",  POLL_IN,  CONST_CS | CONST_PERSISTENT);
#endif
#ifdef POLL_OUT
	REGISTER_LONG_CONSTANT("POLL_OUT", POLL_OUT, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef POLL_MSG
	REGISTER_LONG_CONSTANT("POLL_MSG", POLL_MSG, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef POLL_ERR
	REGISTER_LONG_CONSTANT("POLL_ERR", POLL_ERR, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef POLL_PRI
	REGISTER_LONG_CONSTANT("POLL_PRI", POLL_PRI, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef POLL_HUP
	REGISTER_LONG_CONSTANT("POLL_HUP", POLL_HUP, CONST_CS | CONST_PERSISTENT);
#endif

#ifdef ILL_ILLOPC
	REGISTER_LONG_CONSTANT("ILL_ILLOPC", ILL_ILLOPC, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef ILL_ILLOPN
	REGISTER_LONG_CONSTANT("ILL_ILLOPN", ILL_ILLOPN, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef ILL_ILLADR
	REGISTER_LONG_CONSTANT("ILL_ILLADR", ILL_ILLADR, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef ILL_ILLTRP
	REGISTER_LONG_CONSTANT("ILL_ILLTRP", ILL_ILLTRP, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef ILL_PRVOPC
	REGISTER_LONG_CONSTANT("ILL_PRVOPC", ILL_PRVOPC, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef ILL_PRVREG
	REGISTER_LONG_CONSTANT("ILL_PRVREG", ILL_PRVREG, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef ILL_COPROC
	REGISTER_LONG_CONSTANT("ILL_COPROC", ILL_COPROC, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef ILL_BADSTK
	REGISTER_LONG_CONSTANT("ILL_BADSTK", ILL_BADSTK, CONST_CS | CONST_PERSISTENT);
#endif

#ifdef FPE_INTDIV
	REGISTER_LONG_CONSTANT("FPE_INTDIV", FPE_INTDIV, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef FPE_INTOVF
	REGISTER_LONG_CONSTANT("FPE_INTOVF", FPE_INTOVF, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef FPE_FLTDIV
	REGISTER_LONG_CONSTANT("FPE_FLTDIV", FPE_FLTDIV, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef FPE_FLTOVF
	REGISTER_LONG_CONSTANT("FPE_FLTOVF", FPE_FLTOVF, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef FPE_FLTUND
	REGISTER_LONG_CONSTANT("FPE_FLTUND", FPE_FLTINV, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef FPE_FLTRES
	REGISTER_LONG_CONSTANT("FPE_FLTRES", FPE_FLTRES, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef FPE_FLTINV
	REGISTER_LONG_CONSTANT("FPE_FLTINV", FPE_FLTINV, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef FPE_FLTSUB
	REGISTER_LONG_CONSTANT("FPE_FLTSUB", FPE_FLTSUB, CONST_CS | CONST_PERSISTENT);
#endif

#ifdef SEGV_MAPERR
	REGISTER_LONG_CONSTANT("SEGV_MAPERR", SEGV_MAPERR, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef SEGV_ACCERR
	REGISTER_LONG_CONSTANT("SEGV_ACCERR", SEGV_ACCERR, CONST_CS | CONST_PERSISTENT);
#endif

#ifdef BUS_ADRALN
	REGISTER_LONG_CONSTANT("BUS_ADRALN", BUS_ADRALN, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef BUS_ADRERR
	REGISTER_LONG_CONSTANT("BUS_ADRERR", BUS_ADRERR, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef BUS_OBJERR
	REGISTER_LONG_CONSTANT("BUS_OBJERR", BUS_OBJERR, CONST_CS | CONST_PERSISTENT);
#endif
#endif /* HAVE_SIGWAITINFO && HAVE_SIGTIMEDWAIT */
	/* }}} */
}