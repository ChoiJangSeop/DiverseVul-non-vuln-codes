static int snd_ctl_elem_user_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct user_element *ue = kcontrol->private_data;

	memcpy(&ucontrol->value, ue->elem_data, ue->elem_data_size);
	return 0;
}