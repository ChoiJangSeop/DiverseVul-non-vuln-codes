static void sas_init_port(struct asd_sas_port *port,
			  struct sas_ha_struct *sas_ha, int i)
{
	memset(port, 0, sizeof(*port));
	port->id = i;
	INIT_LIST_HEAD(&port->dev_list);
	INIT_LIST_HEAD(&port->disco_list);
	INIT_LIST_HEAD(&port->destroy_list);
	spin_lock_init(&port->phy_list_lock);
	INIT_LIST_HEAD(&port->phy_list);
	port->ha = sas_ha;

	spin_lock_init(&port->dev_list_lock);
}