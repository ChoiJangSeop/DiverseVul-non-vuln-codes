int hns_ppe_get_sset_count(int stringset)
{
	if (stringset == ETH_SS_STATS)
		return ETH_PPE_STATIC_NUM;
	return 0;
}