long __sys_recvmsg_sock(struct socket *sock, struct user_msghdr __user *msg,
			unsigned int flags)
{
	struct msghdr msg_sys;

	return ___sys_recvmsg(sock, msg, &msg_sys, flags, 0);
}