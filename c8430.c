long __sys_sendmsg_sock(struct socket *sock, struct user_msghdr __user *msg,
			unsigned int flags)
{
	struct msghdr msg_sys;

	return ___sys_sendmsg(sock, msg, &msg_sys, flags, NULL, 0);
}