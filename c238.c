static int tvaudio_get_ctrl(struct CHIPSTATE *chip,
			    struct v4l2_control *ctrl)
{
	struct CHIPDESC *desc = chip->desc;

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_MUTE:
		ctrl->value=chip->muted;
		return 0;
	case V4L2_CID_AUDIO_VOLUME:
		if (!(desc->flags & CHIP_HAS_VOLUME))
			break;
		ctrl->value = max(chip->left,chip->right);
		return 0;
	case V4L2_CID_AUDIO_BALANCE:
	{
		int volume;
		if (!(desc->flags & CHIP_HAS_VOLUME))
			break;
		volume = max(chip->left,chip->right);
		if (volume)
			ctrl->value=(32768*min(chip->left,chip->right))/volume;
		else
			ctrl->value=32768;
		return 0;
	}
	case V4L2_CID_AUDIO_BASS:
		if (desc->flags & CHIP_HAS_BASSTREBLE)
			break;
		ctrl->value = chip->bass;
		return 0;
	case V4L2_CID_AUDIO_TREBLE:
		if (desc->flags & CHIP_HAS_BASSTREBLE)
			return -EINVAL;
		ctrl->value = chip->treble;
		return 0;
	}
	return -EINVAL;
}