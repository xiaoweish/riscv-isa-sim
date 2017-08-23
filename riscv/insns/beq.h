if(RS1.data == RS2.data) {
   word_t tagctrl, tag;
   get_tagctrl(tagctrl);
   tag = get_field(tagctrl, TMASK_CFLOW_DIR_TGT);
   set_pc(BRANCH_TARGET);
   npc.tag = tag;
}
