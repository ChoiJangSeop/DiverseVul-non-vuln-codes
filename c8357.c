int lg3ff_init(struct hid_device *hid)
{
	struct hid_input *hidinput = list_entry(hid->inputs.next, struct hid_input, list);
	struct input_dev *dev = hidinput->input;
	const signed short *ff_bits = ff3_joystick_ac;
	int error;
	int i;

	/* Check that the report looks ok */
	if (!hid_validate_values(hid, HID_OUTPUT_REPORT, 0, 0, 35))
		return -ENODEV;

	/* Assume single fixed device G940 */
	for (i = 0; ff_bits[i] >= 0; i++)
		set_bit(ff_bits[i], dev->ffbit);

	error = input_ff_create_memless(dev, NULL, hid_lg3ff_play);
	if (error)
		return error;

	if (test_bit(FF_AUTOCENTER, dev->ffbit))
		dev->ff->set_autocenter = hid_lg3ff_set_autocenter;

	hid_info(hid, "Force feedback for Logitech Flight System G940 by Gary Stein <LordCnidarian@gmail.com>\n");
	return 0;
}