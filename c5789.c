static int __fpu__restore_sig(void __user *buf, void __user *buf_fx, int size)
{
	int ia32_fxstate = (buf != buf_fx);
	struct task_struct *tsk = current;
	struct fpu *fpu = &tsk->thread.fpu;
	int state_size = fpu_kernel_xstate_size;
	u64 xfeatures = 0;
	int fx_only = 0;

	ia32_fxstate &= (IS_ENABLED(CONFIG_X86_32) ||
			 IS_ENABLED(CONFIG_IA32_EMULATION));

	if (!buf) {
		fpu__clear(fpu);
		return 0;
	}

	if (!access_ok(VERIFY_READ, buf, size))
		return -EACCES;

	fpu__activate_curr(fpu);

	if (!static_cpu_has(X86_FEATURE_FPU))
		return fpregs_soft_set(current, NULL,
				       0, sizeof(struct user_i387_ia32_struct),
				       NULL, buf) != 0;

	if (use_xsave()) {
		struct _fpx_sw_bytes fx_sw_user;
		if (unlikely(check_for_xstate(buf_fx, buf_fx, &fx_sw_user))) {
			/*
			 * Couldn't find the extended state information in the
			 * memory layout. Restore just the FP/SSE and init all
			 * the other extended state.
			 */
			state_size = sizeof(struct fxregs_state);
			fx_only = 1;
			trace_x86_fpu_xstate_check_failed(fpu);
		} else {
			state_size = fx_sw_user.xstate_size;
			xfeatures = fx_sw_user.xfeatures;
		}
	}

	if (ia32_fxstate) {
		/*
		 * For 32-bit frames with fxstate, copy the user state to the
		 * thread's fpu state, reconstruct fxstate from the fsave
		 * header. Sanitize the copied state etc.
		 */
		struct fpu *fpu = &tsk->thread.fpu;
		struct user_i387_ia32_struct env;
		int err = 0;

		/*
		 * Drop the current fpu which clears fpu->fpstate_active. This ensures
		 * that any context-switch during the copy of the new state,
		 * avoids the intermediate state from getting restored/saved.
		 * Thus avoiding the new restored state from getting corrupted.
		 * We will be ready to restore/save the state only after
		 * fpu->fpstate_active is again set.
		 */
		fpu__drop(fpu);

		if (using_compacted_format())
			err = copy_user_to_xstate(&fpu->state.xsave, buf_fx);
		else
			err = __copy_from_user(&fpu->state.xsave, buf_fx, state_size);

		if (err || __copy_from_user(&env, buf, sizeof(env))) {
			fpstate_init(&fpu->state);
			trace_x86_fpu_init_state(fpu);
			err = -1;
		} else {
			sanitize_restored_xstate(tsk, &env, xfeatures, fx_only);
		}

		fpu->fpstate_active = 1;
		preempt_disable();
		fpu__restore(fpu);
		preempt_enable();

		return err;
	} else {
		/*
		 * For 64-bit frames and 32-bit fsave frames, restore the user
		 * state to the registers directly (with exceptions handled).
		 */
		user_fpu_begin();
		if (copy_user_to_fpregs_zeroing(buf_fx, xfeatures, fx_only)) {
			fpu__clear(fpu);
			return -1;
		}
	}

	return 0;
}