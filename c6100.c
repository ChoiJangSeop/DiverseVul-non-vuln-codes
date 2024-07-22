int sas_discover_sata(struct domain_device *dev)
{
	int res;

	if (dev->dev_type == SAS_SATA_PM)
		return -ENODEV;

	dev->sata_dev.class = sas_get_ata_command_set(dev);
	sas_fill_in_rphy(dev, dev->rphy);

	res = sas_notify_lldd_dev_found(dev);
	if (res)
		return res;

	sas_discover_event(dev->port, DISCE_PROBE);
	return 0;
}