ConvertTimeZoneAbbrevs(TimeZoneAbbrevTable *tbl,
					   struct tzEntry *abbrevs, int n)
{
	datetkn    *newtbl = tbl->abbrevs;
	int			i;

	tbl->numabbrevs = n;
	for (i = 0; i < n; i++)
	{
		strcpy_s(newtbl[i].token, TOKMAXLEN, abbrevs[i].abbrev);
		newtbl[i].type = abbrevs[i].is_dst ? DTZ : TZ;
		TOVAL(&newtbl[i], abbrevs[i].offset / MINS_PER_HOUR);
	}

	/* Check the ordering, if testing */
	Assert(CheckDateTokenTable("timezone offset", newtbl, n));
}