static int tt_s2_4600_frontend_attach(struct dvb_usb_adapter *adap)
{
	struct dvb_usb_device *d = adap->dev;
	struct dw2102_state *state = d->priv;
	u8 obuf[3] = { 0xe, 0x80, 0 };
	u8 ibuf[] = { 0 };
	struct i2c_adapter *i2c_adapter;
	struct i2c_client *client;
	struct i2c_board_info board_info;
	struct m88ds3103_platform_data m88ds3103_pdata = {};
	struct ts2020_config ts2020_config = {};

	if (dvb_usb_generic_rw(d, obuf, 3, ibuf, 1, 0) < 0)
		err("command 0x0e transfer failed.");

	obuf[0] = 0xe;
	obuf[1] = 0x02;
	obuf[2] = 1;

	if (dvb_usb_generic_rw(d, obuf, 3, ibuf, 1, 0) < 0)
		err("command 0x0e transfer failed.");
	msleep(300);

	obuf[0] = 0xe;
	obuf[1] = 0x83;
	obuf[2] = 0;

	if (dvb_usb_generic_rw(d, obuf, 3, ibuf, 1, 0) < 0)
		err("command 0x0e transfer failed.");

	obuf[0] = 0xe;
	obuf[1] = 0x83;
	obuf[2] = 1;

	if (dvb_usb_generic_rw(d, obuf, 3, ibuf, 1, 0) < 0)
		err("command 0x0e transfer failed.");

	obuf[0] = 0x51;

	if (dvb_usb_generic_rw(d, obuf, 1, ibuf, 1, 0) < 0)
		err("command 0x51 transfer failed.");

	/* attach demod */
	m88ds3103_pdata.clk = 27000000;
	m88ds3103_pdata.i2c_wr_max = 33;
	m88ds3103_pdata.ts_mode = M88DS3103_TS_CI;
	m88ds3103_pdata.ts_clk = 16000;
	m88ds3103_pdata.ts_clk_pol = 0;
	m88ds3103_pdata.spec_inv = 0;
	m88ds3103_pdata.agc = 0x99;
	m88ds3103_pdata.agc_inv = 0;
	m88ds3103_pdata.clk_out = M88DS3103_CLOCK_OUT_ENABLED;
	m88ds3103_pdata.envelope_mode = 0;
	m88ds3103_pdata.lnb_hv_pol = 1;
	m88ds3103_pdata.lnb_en_pol = 0;
	memset(&board_info, 0, sizeof(board_info));
	strlcpy(board_info.type, "m88ds3103", I2C_NAME_SIZE);
	board_info.addr = 0x68;
	board_info.platform_data = &m88ds3103_pdata;
	request_module("m88ds3103");
	client = i2c_new_device(&d->i2c_adap, &board_info);
	if (client == NULL || client->dev.driver == NULL)
		return -ENODEV;
	if (!try_module_get(client->dev.driver->owner)) {
		i2c_unregister_device(client);
		return -ENODEV;
	}
	adap->fe_adap[0].fe = m88ds3103_pdata.get_dvb_frontend(client);
	i2c_adapter = m88ds3103_pdata.get_i2c_adapter(client);

	state->i2c_client_demod = client;

	/* attach tuner */
	ts2020_config.fe = adap->fe_adap[0].fe;
	memset(&board_info, 0, sizeof(board_info));
	strlcpy(board_info.type, "ts2022", I2C_NAME_SIZE);
	board_info.addr = 0x60;
	board_info.platform_data = &ts2020_config;
	request_module("ts2020");
	client = i2c_new_device(i2c_adapter, &board_info);

	if (client == NULL || client->dev.driver == NULL) {
		dvb_frontend_detach(adap->fe_adap[0].fe);
		return -ENODEV;
	}

	if (!try_module_get(client->dev.driver->owner)) {
		i2c_unregister_device(client);
		dvb_frontend_detach(adap->fe_adap[0].fe);
		return -ENODEV;
	}

	/* delegate signal strength measurement to tuner */
	adap->fe_adap[0].fe->ops.read_signal_strength =
			adap->fe_adap[0].fe->ops.tuner_ops.get_rf_strength;

	state->i2c_client_tuner = client;

	/* hook fe: need to resync the slave fifo when signal locks */
	state->fe_read_status = adap->fe_adap[0].fe->ops.read_status;
	adap->fe_adap[0].fe->ops.read_status = tt_s2_4600_read_status;

	state->last_lock = 0;

	return 0;
}