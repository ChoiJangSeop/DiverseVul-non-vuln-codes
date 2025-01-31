void perf_event_disable(struct perf_event *event)
{
	struct perf_event_context *ctx = event->ctx;
	struct task_struct *task = ctx->task;

	if (!task) {
		/*
		 * Disable the event on the cpu that it's on
		 */
		cpu_function_call(event->cpu, __perf_event_disable, event);
		return;
	}

retry:
	if (!task_function_call(task, __perf_event_disable, event))
		return;

	raw_spin_lock_irq(&ctx->lock);
	/*
	 * If the event is still active, we need to retry the cross-call.
	 */
	if (event->state == PERF_EVENT_STATE_ACTIVE) {
		raw_spin_unlock_irq(&ctx->lock);
		/*
		 * Reload the task pointer, it might have been changed by
		 * a concurrent perf_event_context_sched_out().
		 */
		task = ctx->task;
		goto retry;
	}

	/*
	 * Since we have the lock this context can't be scheduled
	 * in, so we can change the state safely.
	 */
	if (event->state == PERF_EVENT_STATE_INACTIVE) {
		update_group_times(event);
		event->state = PERF_EVENT_STATE_OFF;
	}
	raw_spin_unlock_irq(&ctx->lock);
}