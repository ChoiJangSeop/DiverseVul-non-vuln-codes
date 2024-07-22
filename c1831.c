  BoundsCheckBbData(BoundsCheckKey* key,
                    int32_t lower_offset,
                    int32_t upper_offset,
                    HBasicBlock* bb,
                    HBoundsCheck* lower_check,
                    HBoundsCheck* upper_check,
                    BoundsCheckBbData* next_in_bb,
                    BoundsCheckBbData* father_in_dt)
  : key_(key),
    lower_offset_(lower_offset),
    upper_offset_(upper_offset),
    basic_block_(bb),
    lower_check_(lower_check),
    upper_check_(upper_check),
    added_lower_index_(NULL),
    added_lower_offset_(NULL),
    added_upper_index_(NULL),
    added_upper_offset_(NULL),
    next_in_bb_(next_in_bb),
    father_in_dt_(father_in_dt) { }