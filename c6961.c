static int parseOperand(RAsm *a, const char *str, Operand *op, bool isrepop) {
	size_t pos, nextpos = 0;
	x86newTokenType last_type;
	int size_token = 1;
	bool explicit_size = false;
	int reg_index = 0;
	// Reset type
	op->type = 0;
	// Consume tokens denoting the operand size
	while (size_token) {
		pos = nextpos;
		last_type = getToken (str, &pos, &nextpos);

		// Token may indicate size: then skip
		if (!r_str_ncasecmp (str + pos, "ptr", 3)) {
			continue;
		} else if (!r_str_ncasecmp (str + pos, "byte", 4)) {
			op->type |= OT_MEMORY | OT_BYTE;
			op->dest_size = OT_BYTE;
			explicit_size = true;
		} else if (!r_str_ncasecmp (str + pos, "word", 4)) {
			op->type |= OT_MEMORY | OT_WORD;
			op->dest_size = OT_WORD;
			explicit_size = true;
		} else if (!r_str_ncasecmp (str + pos, "dword", 5)) {
			op->type |= OT_MEMORY | OT_DWORD;
			op->dest_size = OT_DWORD;
			explicit_size = true;
		} else if (!r_str_ncasecmp (str + pos, "qword", 5)) {
			op->type |= OT_MEMORY | OT_QWORD;
			op->dest_size = OT_QWORD;
			explicit_size = true;
		} else if (!r_str_ncasecmp (str + pos, "oword", 5)) {
			op->type |= OT_MEMORY | OT_OWORD;
			op->dest_size = OT_OWORD;
			explicit_size = true;
		} else if (!r_str_ncasecmp (str + pos, "tbyte", 5)) {
			op->type |= OT_MEMORY | OT_TBYTE;
			op->dest_size = OT_TBYTE;
			explicit_size = true;
		} else { // the current token doesn't denote a size
			size_token = 0;
		}
	}

	// Next token: register, immediate, or '['
	if (str[pos] == '[') {
		// Don't care about size, if none is given.
		if (!op->type) {
			op->type = OT_MEMORY;
		}
		// At the moment, we only accept plain linear combinations:
		// part := address | [factor *] register
		// address := part {+ part}*
		op->offset = op->scale[0] = op->scale[1] = 0;

		ut64 temp = 1;
		Register reg = X86R_UNDEFINED;
		bool first_reg = true;
		while (str[pos] != ']') {
			if (pos > nextpos) {
			//	eprintf ("Error parsing instruction\n");
				break;
			}
			pos = nextpos;
			if (!str[pos]) {
				break;
			}
			last_type = getToken (str, &pos, &nextpos);

			if (last_type == TT_SPECIAL) {
				if (str[pos] == '+' || str[pos] == '-' || str[pos] == ']') {
					if (reg != X86R_UNDEFINED) {
						op->regs[reg_index] = reg;
						op->scale[reg_index] = temp;
						++reg_index;
					} else {
						op->offset += temp;
						op->regs[reg_index] = X86R_UNDEFINED;
					}

					temp = 1;
					reg = X86R_UNDEFINED;
				} else if (str[pos] == '*') {
					// go to ], + or - to get scale

					// Something to do here?
					// Seems we are just ignoring '*' or assuming it implicitly.
				}
			}
			else if (last_type == TT_WORD) {
				ut32 reg_type = 0;

				// We can't multiply registers
				if (reg != X86R_UNDEFINED) {
					op->type = 0;	// Make the result invalid
				}

				// Reset nextpos: parseReg wants to parse from the beginning
				nextpos = pos;
				reg = parseReg (a, str, &nextpos, &reg_type);

				if (first_reg) {
					op->extended = false;
					if (reg > 8) {
						op->extended = true;
						op->reg = reg - 9;
					}
					first_reg = false;
				} else if (reg > 8) {
					op->reg = reg - 9;
				}
				if (reg_type & OT_REGTYPE & OT_SEGMENTREG) {
					op->reg = reg;
					op->type = reg_type;
					parse_segment_offset (a, str, &nextpos, op, reg_index);
					return nextpos;
				}

				// Still going to need to know the size if not specified
				if (!explicit_size) {
					op->type |= reg_type;
				}
				op->reg_size = reg_type;
				op->explicit_size = explicit_size;

				// Addressing only via general purpose registers
				if (!(reg_type & OT_GPREG)) {
					op->type = 0;	// Make the result invalid
				}
			}
			else {
				char *p = strchr (str, '+');
				op->offset_sign = 1;
				if (!p) {
					p = strchr (str, '-');
					if (p) {
						op->offset_sign = -1;
					}
				}
				//with SIB notation, we need to consider the right sign
				char * plus = strchr (str, '+');
				char * minus = strchr (str, '-');
				char * closeB = strchr (str, ']');
				if (plus && minus && plus < closeB && minus < closeB) {
					op->offset_sign = -1;
				}
				// If there's a scale, we don't want to parse out the
				// scale with the offset (scale + offset) otherwise the scale
				// will be the sum of the two. This splits the numbers
				char *tmp;
				tmp = malloc (strlen (str + pos) + 1);
				strcpy (tmp, str + pos);
				strtok (tmp, "+-");
				st64 read = getnum (a, tmp);
				free (tmp);
				temp *= read;
			}
		}
	} else if (last_type == TT_WORD) {   // register
		nextpos = pos;
		RFlagItem *flag;

		if (isrepop) {
			op->is_good_flag = false;
			strncpy (op->rep_op, str, MAX_REPOP_LENGTH - 1);
			op->rep_op[MAX_REPOP_LENGTH - 1] = '\0';
			return nextpos;
		}

		op->reg = parseReg (a, str, &nextpos, &op->type);

		op->extended = false;
		if (op->reg > 8) {
			op->extended = true;
			op->reg -= 9;
		}
		if (op->type & OT_REGTYPE & OT_SEGMENTREG) {
			parse_segment_offset (a, str, &nextpos, op, reg_index);
			return nextpos;
		}
		if (op->reg == X86R_UNDEFINED) {
			op->is_good_flag = false;
			if (a->num && a->num->value == 0) {
				return nextpos;
			}
			op->type = OT_CONSTANT;
			RCore *core = a->num? (RCore *)(a->num->userptr): NULL;
			if (core && (flag = r_flag_get (core->flags, str))) {
				op->is_good_flag = true;
			}

			char *p = strchr (str, '-');
			if (p) {
				op->sign = -1;
				str = ++p;
			}
			op->immediate = getnum (a, str);
		} else if (op->reg < X86R_UNDEFINED) {
			strncpy (op->rep_op, str, MAX_REPOP_LENGTH - 1);
			op->rep_op[MAX_REPOP_LENGTH - 1] = '\0';
		}
	} else {                             // immediate
		// We don't know the size, so let's just set no size flag.
		op->type = OT_CONSTANT;
		op->sign = 1;
		char *p = strchr (str, '-');
		if (p) {
			op->sign = -1;
			str = ++p;
		}
		op->immediate = getnum (a, str);
	}

	return nextpos;
}