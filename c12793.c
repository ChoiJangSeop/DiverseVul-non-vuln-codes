static int linear_ioctl(struct dm_target *ti, unsigned int cmd,
			unsigned long arg)
{
	struct linear_c *lc = (struct linear_c *) ti->private;
	return __blkdev_driver_ioctl(lc->dev->bdev, lc->dev->mode, cmd, arg);
}