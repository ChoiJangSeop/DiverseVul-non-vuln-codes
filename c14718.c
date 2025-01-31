static void put_prev_task(struct rq *rq, struct task_struct *prev)
{
	if (prev->se.on_rq)
		update_rq_clock(rq);
	rq->skip_clock_update = 0;
	prev->sched_class->put_prev_task(rq, prev);
}