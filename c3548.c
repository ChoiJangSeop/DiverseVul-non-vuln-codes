static int snd_hrtimer_start(struct snd_timer *t)
{
	struct snd_hrtimer *stime = t->private_data;

	atomic_set(&stime->running, 0);
	hrtimer_cancel(&stime->hrt);
	hrtimer_start(&stime->hrt, ns_to_ktime(t->sticks * resolution),
		      HRTIMER_MODE_REL);
	atomic_set(&stime->running, 1);
	return 0;
}