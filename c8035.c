static int build_audio_procunit(struct mixer_build *state, int unitid,
				void *raw_desc, struct procunit_info *list,
				char *name)
{
	struct uac_processing_unit_descriptor *desc = raw_desc;
	int num_ins = desc->bNrInPins;
	struct usb_mixer_elem_info *cval;
	struct snd_kcontrol *kctl;
	int i, err, nameid, type, len;
	struct procunit_info *info;
	struct procunit_value_info *valinfo;
	const struct usbmix_name_map *map;
	static struct procunit_value_info default_value_info[] = {
		{ 0x01, "Switch", USB_MIXER_BOOLEAN },
		{ 0 }
	};
	static struct procunit_info default_info = {
		0, NULL, default_value_info
	};

	if (desc->bLength < 13 || desc->bLength < 13 + num_ins ||
	    desc->bLength < num_ins + uac_processing_unit_bControlSize(desc, state->mixer->protocol)) {
		usb_audio_err(state->chip, "invalid %s descriptor (id %d)\n", name, unitid);
		return -EINVAL;
	}

	for (i = 0; i < num_ins; i++) {
		err = parse_audio_unit(state, desc->baSourceID[i]);
		if (err < 0)
			return err;
	}

	type = le16_to_cpu(desc->wProcessType);
	for (info = list; info && info->type; info++)
		if (info->type == type)
			break;
	if (!info || !info->type)
		info = &default_info;

	for (valinfo = info->values; valinfo->control; valinfo++) {
		__u8 *controls = uac_processing_unit_bmControls(desc, state->mixer->protocol);

		if (state->mixer->protocol == UAC_VERSION_1) {
			if (!(controls[valinfo->control / 8] &
					(1 << ((valinfo->control % 8) - 1))))
				continue;
		} else { /* UAC_VERSION_2/3 */
			if (!uac_v2v3_control_is_readable(controls[valinfo->control / 8],
							  valinfo->control))
				continue;
		}

		map = find_map(state->map, unitid, valinfo->control);
		if (check_ignored_ctl(map))
			continue;
		cval = kzalloc(sizeof(*cval), GFP_KERNEL);
		if (!cval)
			return -ENOMEM;
		snd_usb_mixer_elem_init_std(&cval->head, state->mixer, unitid);
		cval->control = valinfo->control;
		cval->val_type = valinfo->val_type;
		cval->channels = 1;

		if (state->mixer->protocol > UAC_VERSION_1 &&
		    !uac_v2v3_control_is_writeable(controls[valinfo->control / 8],
						   valinfo->control))
			cval->master_readonly = 1;

		/* get min/max values */
		switch (type) {
		case UAC_PROCESS_UP_DOWNMIX: {
			bool mode_sel = false;

			switch (state->mixer->protocol) {
			case UAC_VERSION_1:
			case UAC_VERSION_2:
			default:
				if (cval->control == UAC_UD_MODE_SELECT)
					mode_sel = true;
				break;
			case UAC_VERSION_3:
				if (cval->control == UAC3_UD_MODE_SELECT)
					mode_sel = true;
				break;
			}

			if (mode_sel) {
				__u8 *control_spec = uac_processing_unit_specific(desc,
								state->mixer->protocol);
				cval->min = 1;
				cval->max = control_spec[0];
				cval->res = 1;
				cval->initialized = 1;
				break;
			}

			get_min_max(cval, valinfo->min_value);
			break;
		}
		case USB_XU_CLOCK_RATE:
			/*
			 * E-Mu USB 0404/0202/TrackerPre/0204
			 * samplerate control quirk
			 */
			cval->min = 0;
			cval->max = 5;
			cval->res = 1;
			cval->initialized = 1;
			break;
		default:
			get_min_max(cval, valinfo->min_value);
			break;
		}

		kctl = snd_ctl_new1(&mixer_procunit_ctl, cval);
		if (!kctl) {
			kfree(cval);
			return -ENOMEM;
		}
		kctl->private_free = snd_usb_mixer_elem_free;

		if (check_mapped_name(map, kctl->id.name, sizeof(kctl->id.name))) {
			/* nothing */ ;
		} else if (info->name) {
			strlcpy(kctl->id.name, info->name, sizeof(kctl->id.name));
		} else {
			nameid = uac_processing_unit_iProcessing(desc, state->mixer->protocol);
			len = 0;
			if (nameid)
				len = snd_usb_copy_string_desc(state->chip,
							       nameid,
							       kctl->id.name,
							       sizeof(kctl->id.name));
			if (!len)
				strlcpy(kctl->id.name, name, sizeof(kctl->id.name));
		}
		append_ctl_name(kctl, " ");
		append_ctl_name(kctl, valinfo->suffix);

		usb_audio_dbg(state->chip,
			      "[%d] PU [%s] ch = %d, val = %d/%d\n",
			      cval->head.id, kctl->id.name, cval->channels,
			      cval->min, cval->max);

		err = snd_usb_mixer_add_control(&cval->head, kctl);
		if (err < 0)
			return err;
	}
	return 0;
}