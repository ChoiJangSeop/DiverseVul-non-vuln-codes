static int crypto_ahash_report(struct sk_buff *skb, struct crypto_alg *alg)
{
	struct crypto_report_hash rhash;

	snprintf(rhash.type, CRYPTO_MAX_ALG_NAME, "%s", "ahash");

	rhash.blocksize = alg->cra_blocksize;
	rhash.digestsize = __crypto_hash_alg_common(alg)->digestsize;

	if (nla_put(skb, CRYPTOCFGA_REPORT_HASH,
		    sizeof(struct crypto_report_hash), &rhash))
		goto nla_put_failure;
	return 0;

nla_put_failure:
	return -EMSGSIZE;
}