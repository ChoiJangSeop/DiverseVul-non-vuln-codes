static void sas_resume_port(struct asd_sas_phy *phy)
{
	struct domain_device *dev;
	struct asd_sas_port *port = phy->port;
	struct sas_ha_struct *sas_ha = phy->ha;
	struct sas_internal *si = to_sas_internal(sas_ha->core.shost->transportt);

	if (si->dft->lldd_port_formed)
		si->dft->lldd_port_formed(phy);

	if (port->suspended)
		port->suspended = 0;
	else {
		/* we only need to handle "link returned" actions once */
		return;
	}

	/* if the port came back:
	 * 1/ presume every device came back
	 * 2/ force the next revalidation to check all expander phys
	 */
	list_for_each_entry(dev, &port->dev_list, dev_list_node) {
		int i, rc;

		rc = sas_notify_lldd_dev_found(dev);
		if (rc) {
			sas_unregister_dev(port, dev);
			continue;
		}

		if (dev->dev_type == SAS_EDGE_EXPANDER_DEVICE || dev->dev_type == SAS_FANOUT_EXPANDER_DEVICE) {
			dev->ex_dev.ex_change_count = -1;
			for (i = 0; i < dev->ex_dev.num_phys; i++) {
				struct ex_phy *phy = &dev->ex_dev.ex_phy[i];

				phy->phy_change_count = -1;
			}
		}
	}

	sas_discover_event(port, DISCE_RESUME);
}