void __init init_timers(void)
{
	init_timer_cpus();
	init_timer_stats();
	open_softirq(TIMER_SOFTIRQ, run_timer_softirq);
}