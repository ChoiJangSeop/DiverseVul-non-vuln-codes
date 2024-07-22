static int cqspi_setup_flash(struct cqspi_st *cqspi, struct device_node *np)
{
	struct platform_device *pdev = cqspi->pdev;
	struct device *dev = &pdev->dev;
	struct cqspi_flash_pdata *f_pdata;
	struct spi_nor *nor;
	struct mtd_info *mtd;
	unsigned int cs;
	int i, ret;

	/* Get flash device data */
	for_each_available_child_of_node(dev->of_node, np) {
		if (of_property_read_u32(np, "reg", &cs)) {
			dev_err(dev, "Couldn't determine chip select.\n");
			goto err;
		}

		if (cs > CQSPI_MAX_CHIPSELECT) {
			dev_err(dev, "Chip select %d out of range.\n", cs);
			goto err;
		}

		f_pdata = &cqspi->f_pdata[cs];
		f_pdata->cqspi = cqspi;
		f_pdata->cs = cs;

		ret = cqspi_of_get_flash_pdata(pdev, f_pdata, np);
		if (ret)
			goto err;

		nor = &f_pdata->nor;
		mtd = &nor->mtd;

		mtd->priv = nor;

		nor->dev = dev;
		spi_nor_set_flash_node(nor, np);
		nor->priv = f_pdata;

		nor->read_reg = cqspi_read_reg;
		nor->write_reg = cqspi_write_reg;
		nor->read = cqspi_read;
		nor->write = cqspi_write;
		nor->erase = cqspi_erase;
		nor->prepare = cqspi_prep;
		nor->unprepare = cqspi_unprep;

		mtd->name = devm_kasprintf(dev, GFP_KERNEL, "%s.%d",
					   dev_name(dev), cs);
		if (!mtd->name) {
			ret = -ENOMEM;
			goto err;
		}

		ret = spi_nor_scan(nor, NULL, SPI_NOR_QUAD);
		if (ret)
			goto err;

		ret = mtd_device_register(mtd, NULL, 0);
		if (ret)
			goto err;

		f_pdata->registered = true;
	}

	return 0;

err:
	for (i = 0; i < CQSPI_MAX_CHIPSELECT; i++)
		if (cqspi->f_pdata[i].registered)
			mtd_device_unregister(&cqspi->f_pdata[i].nor.mtd);
	return ret;
}