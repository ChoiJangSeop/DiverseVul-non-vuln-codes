static bool check_underflow(const struct ipt_entry *e)
{
	const struct xt_entry_target *t;
	unsigned int verdict;

	if (!unconditional(&e->ip))
		return false;
	t = ipt_get_target_c(e);
	if (strcmp(t->u.user.name, XT_STANDARD_TARGET) != 0)
		return false;
	verdict = ((struct xt_standard_target *)t)->verdict;
	verdict = -verdict - 1;
	return verdict == NF_DROP || verdict == NF_ACCEPT;
}