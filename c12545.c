static int crypto_rng_report(struct sk_buff *skb, struct crypto_alg *alg)
{
	struct crypto_report_rng rrng;

	snprintf(rrng.type, CRYPTO_MAX_ALG_NAME, "%s", "rng");

	rrng.seedsize = alg->cra_rng.seedsize;

	if (nla_put(skb, CRYPTOCFGA_REPORT_RNG,
		    sizeof(struct crypto_report_rng), &rrng))
		goto nla_put_failure;
	return 0;

nla_put_failure:
	return -EMSGSIZE;
}