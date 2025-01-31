static void recalculate_apic_map(struct kvm *kvm)
{
	struct kvm_apic_map *new, *old = NULL;
	struct kvm_vcpu *vcpu;
	int i;

	new = kzalloc(sizeof(struct kvm_apic_map), GFP_KERNEL);

	mutex_lock(&kvm->arch.apic_map_lock);

	if (!new)
		goto out;

	new->ldr_bits = 8;
	/* flat mode is default */
	new->cid_shift = 8;
	new->cid_mask = 0;
	new->lid_mask = 0xff;

	kvm_for_each_vcpu(i, vcpu, kvm) {
		struct kvm_lapic *apic = vcpu->arch.apic;
		u16 cid, lid;
		u32 ldr;

		if (!kvm_apic_present(vcpu))
			continue;

		/*
		 * All APICs have to be configured in the same mode by an OS.
		 * We take advatage of this while building logical id loockup
		 * table. After reset APICs are in xapic/flat mode, so if we
		 * find apic with different setting we assume this is the mode
		 * OS wants all apics to be in; build lookup table accordingly.
		 */
		if (apic_x2apic_mode(apic)) {
			new->ldr_bits = 32;
			new->cid_shift = 16;
			new->cid_mask = new->lid_mask = 0xffff;
		} else if (kvm_apic_sw_enabled(apic) &&
				!new->cid_mask /* flat mode */ &&
				kvm_apic_get_reg(apic, APIC_DFR) == APIC_DFR_CLUSTER) {
			new->cid_shift = 4;
			new->cid_mask = 0xf;
			new->lid_mask = 0xf;
		}

		new->phys_map[kvm_apic_id(apic)] = apic;

		ldr = kvm_apic_get_reg(apic, APIC_LDR);
		cid = apic_cluster_id(new, ldr);
		lid = apic_logical_id(new, ldr);

		if (lid)
			new->logical_map[cid][ffs(lid) - 1] = apic;
	}
out:
	old = rcu_dereference_protected(kvm->arch.apic_map,
			lockdep_is_held(&kvm->arch.apic_map_lock));
	rcu_assign_pointer(kvm->arch.apic_map, new);
	mutex_unlock(&kvm->arch.apic_map_lock);

	if (old)
		kfree_rcu(old, rcu);

	kvm_vcpu_request_scan_ioapic(kvm);
}