static void pcd_init_units(void)
{
	struct pcd_unit *cd;
	int unit;

	pcd_drive_count = 0;
	for (unit = 0, cd = pcd; unit < PCD_UNITS; unit++, cd++) {
		struct gendisk *disk = alloc_disk(1);

		if (!disk)
			continue;

		disk->queue = blk_mq_init_sq_queue(&cd->tag_set, &pcd_mq_ops,
						   1, BLK_MQ_F_SHOULD_MERGE);
		if (IS_ERR(disk->queue)) {
			disk->queue = NULL;
			continue;
		}

		INIT_LIST_HEAD(&cd->rq_list);
		disk->queue->queuedata = cd;
		blk_queue_bounce_limit(disk->queue, BLK_BOUNCE_HIGH);
		cd->disk = disk;
		cd->pi = &cd->pia;
		cd->present = 0;
		cd->last_sense = 0;
		cd->changed = 1;
		cd->drive = (*drives[unit])[D_SLV];
		if ((*drives[unit])[D_PRT])
			pcd_drive_count++;

		cd->name = &cd->info.name[0];
		snprintf(cd->name, sizeof(cd->info.name), "%s%d", name, unit);
		cd->info.ops = &pcd_dops;
		cd->info.handle = cd;
		cd->info.speed = 0;
		cd->info.capacity = 1;
		cd->info.mask = 0;
		disk->major = major;
		disk->first_minor = unit;
		strcpy_s(disk->disk_name, sizeof(disk->disk_name), cd->name);	/* umm... */
		disk->fops = &pcd_bdops;
		disk->flags = GENHD_FL_BLOCK_EVENTS_ON_EXCL_WRITE;
	}
}