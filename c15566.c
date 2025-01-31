static int pvc_getname(struct socket *sock, struct sockaddr *sockaddr,
		       int *sockaddr_len, int peer)
{
	struct sockaddr_atmpvc *addr;
	struct atm_vcc *vcc = ATM_SD(sock);

	if (!vcc->dev || !test_bit(ATM_VF_ADDR, &vcc->flags))
		return -ENOTCONN;
	*sockaddr_len = sizeof(struct sockaddr_atmpvc);
	addr = (struct sockaddr_atmpvc *)sockaddr;
	addr->sap_family = AF_ATMPVC;
	addr->sap_addr.itf = vcc->dev->number;
	addr->sap_addr.vpi = vcc->vpi;
	addr->sap_addr.vci = vcc->vci;
	return 0;
}