NORET_TYPE void do_exit(long code)
{
	struct task_struct *tsk = current;
	int group_dead;

	profile_task_exit(tsk);

	WARN_ON(atomic_read(&tsk->fs_excl));

	if (unlikely(in_interrupt()))
		panic("Aiee, killing interrupt handler!");
	if (unlikely(!tsk->pid))
		panic("Attempted to kill the idle task!");

	tracehook_report_exit(&code);

	/*
	 * We're taking recursive faults here in do_exit. Safest is to just
	 * leave this task alone and wait for reboot.
	 */
	if (unlikely(tsk->flags & PF_EXITING)) {
		printk(KERN_ALERT
			"Fixing recursive fault but reboot is needed!\n");
		/*
		 * We can do this unlocked here. The futex code uses
		 * this flag just to verify whether the pi state
		 * cleanup has been done or not. In the worst case it
		 * loops once more. We pretend that the cleanup was
		 * done as there is no way to return. Either the
		 * OWNER_DIED bit is set by now or we push the blocked
		 * task into the wait for ever nirwana as well.
		 */
		tsk->flags |= PF_EXITPIDONE;
		if (tsk->io_context)
			exit_io_context();
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule();
	}

	exit_signals(tsk);  /* sets PF_EXITING */
	/*
	 * tsk->flags are checked in the futex code to protect against
	 * an exiting task cleaning up the robust pi futexes.
	 */
	smp_mb();
	spin_unlock_wait(&tsk->pi_lock);

	if (unlikely(in_atomic()))
		printk(KERN_INFO "note: %s[%d] exited with preempt_count %d\n",
				current->comm, task_pid_nr(current),
				preempt_count());

	acct_update_integrals(tsk);
	if (tsk->mm) {
		update_hiwater_rss(tsk->mm);
		update_hiwater_vm(tsk->mm);
	}
	group_dead = atomic_dec_and_test(&tsk->signal->live);
	if (group_dead) {
		hrtimer_cancel(&tsk->signal->real_timer);
		exit_itimers(tsk->signal);
	}
	acct_collect(code, group_dead);
#ifdef CONFIG_FUTEX
	if (unlikely(tsk->robust_list))
		exit_robust_list(tsk);
#ifdef CONFIG_COMPAT
	if (unlikely(tsk->compat_robust_list))
		compat_exit_robust_list(tsk);
#endif
#endif
	if (group_dead)
		tty_audit_exit();
	if (unlikely(tsk->audit_context))
		audit_free(tsk);

	tsk->exit_code = code;
	taskstats_exit(tsk, group_dead);

	exit_mm(tsk);

	if (group_dead)
		acct_process();
	trace_sched_process_exit(tsk);

	exit_sem(tsk);
	exit_files(tsk);
	exit_fs(tsk);
	check_stack_usage();
	exit_thread();
	cgroup_exit(tsk, 1);
	exit_keys(tsk);

	if (group_dead && tsk->signal->leader)
		disassociate_ctty(1);

	module_put(task_thread_info(tsk)->exec_domain->module);
	if (tsk->binfmt)
		module_put(tsk->binfmt->module);

	proc_exit_connector(tsk);
	exit_notify(tsk, group_dead);
#ifdef CONFIG_NUMA
	mpol_put(tsk->mempolicy);
	tsk->mempolicy = NULL;
#endif
#ifdef CONFIG_FUTEX
	/*
	 * This must happen late, after the PID is not
	 * hashed anymore:
	 */
	if (unlikely(!list_empty(&tsk->pi_state_list)))
		exit_pi_state_list(tsk);
	if (unlikely(current->pi_state_cache))
		kfree(current->pi_state_cache);
#endif
	/*
	 * Make sure we are holding no locks:
	 */
	debug_check_no_locks_held(tsk);
	/*
	 * We can do this unlocked here. The futex code uses this flag
	 * just to verify whether the pi state cleanup has been done
	 * or not. In the worst case it loops once more.
	 */
	tsk->flags |= PF_EXITPIDONE;

	if (tsk->io_context)
		exit_io_context();

	if (tsk->splice_pipe)
		__free_pipe_info(tsk->splice_pipe);

	preempt_disable();
	/* causes final put_task_struct in finish_task_switch(). */
	tsk->state = TASK_DEAD;

	schedule();
	BUG();
	/* Avoid "noreturn function does return".  */
	for (;;)
		cpu_relax();	/* For when BUG is null */
}