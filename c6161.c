static void sas_eh_finish_cmd(struct scsi_cmnd *cmd)
{
	struct sas_ha_struct *sas_ha = SHOST_TO_SAS_HA(cmd->device->host);
	struct sas_task *task = TO_SAS_TASK(cmd);

	/* At this point, we only get called following an actual abort
	 * of the task, so we should be guaranteed not to be racing with
	 * any completions from the LLD.  Task is freed after this.
	 */
	sas_end_task(cmd, task);

	/* now finish the command and move it on to the error
	 * handler done list, this also takes it off the
	 * error handler pending list.
	 */
	scsi_eh_finish_cmd(cmd, &sas_ha->eh_done_q);
}