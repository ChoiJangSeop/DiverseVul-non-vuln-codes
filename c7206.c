static void update_blocked_averages(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	struct cfs_rq *cfs_rq, *pos;
	const struct sched_class *curr_class;
	struct rq_flags rf;
	bool done = true;

	rq_lock_irqsave(rq, &rf);
	update_rq_clock(rq);

	/*
	 * Iterates the task_group tree in a bottom up fashion, see
	 * list_add_leaf_cfs_rq() for details.
	 */
	for_each_leaf_cfs_rq_safe(rq, cfs_rq, pos) {
		struct sched_entity *se;

		/* throttled entities do not contribute to load */
		if (throttled_hierarchy(cfs_rq))
			continue;

		if (update_cfs_rq_load_avg(cfs_rq_clock_task(cfs_rq), cfs_rq))
			update_tg_load_avg(cfs_rq, 0);

		/* Propagate pending load changes to the parent, if any: */
		se = cfs_rq->tg->se[cpu];
		if (se && !skip_blocked_update(se))
			update_load_avg(cfs_rq_of(se), se, 0);

		/*
		 * There can be a lot of idle CPU cgroups.  Don't let fully
		 * decayed cfs_rqs linger on the list.
		 */
		if (cfs_rq_is_decayed(cfs_rq))
			list_del_leaf_cfs_rq(cfs_rq);

		/* Don't need periodic decay once load/util_avg are null */
		if (cfs_rq_has_blocked(cfs_rq))
			done = false;
	}

	curr_class = rq->curr->sched_class;
	update_rt_rq_load_avg(rq_clock_task(rq), rq, curr_class == &rt_sched_class);
	update_dl_rq_load_avg(rq_clock_task(rq), rq, curr_class == &dl_sched_class);
	update_irq_load_avg(rq, 0);
	/* Don't need periodic decay once load/util_avg are null */
	if (others_have_blocked(rq))
		done = false;

#ifdef CONFIG_NO_HZ_COMMON
	rq->last_blocked_load_update_tick = jiffies;
	if (done)
		rq->has_blocked_load = 0;
#endif
	rq_unlock_irqrestore(rq, &rf);
}