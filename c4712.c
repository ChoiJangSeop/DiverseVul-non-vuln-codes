static freelist_idx_t next_random_slot(union freelist_init_state *state)
{
	return (state->list[state->pos++] + state->rand) % state->count;
}