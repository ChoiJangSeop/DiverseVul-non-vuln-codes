static char *pool_strdup(const char *s)
{
	char *r = pool_alloc(strlen(s) + 1);
	strcpy_s(r, sizeof(r), s);
	return r;
}