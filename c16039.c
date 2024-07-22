rsvg_new_group (void)
{
    RsvgNodeGroup *group;
    group = g_new (RsvgNodeGroup, 1);
    _rsvg_node_init (&group->super);
    group->super.draw = _rsvg_node_draw_children;
    group->super.set_atts = rsvg_node_group_set_atts;
    return &group->super;
}