cifs_find_smb_ses(struct TCP_Server_Info *server, char *username)
{
	struct list_head *tmp;
	struct cifsSesInfo *ses;

	write_lock(&cifs_tcp_ses_lock);
	list_for_each(tmp, &server->smb_ses_list) {
		ses = list_entry(tmp, struct cifsSesInfo, smb_ses_list);
		if (strncmp(ses->userName, username, MAX_USERNAME_SIZE))
			continue;

		++ses->ses_count;
		write_unlock(&cifs_tcp_ses_lock);
		return ses;
	}
	write_unlock(&cifs_tcp_ses_lock);
	return NULL;
}