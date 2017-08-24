if(RS1.data != RS2.data) {
   word_t tagctrl, tag;
   get_tagctrl(tagctrl);
   tag = get_field(tagctrl, TMASK_CFLOW_DIR_TGT);
   set_pc(BRANCH_TARGET);
   npc.tag = tag;
   if (STATE.prv == PRV_U)
      fprintf(stderr, "pc: 0x%016" PRIx64 " target: 0x%016" PRIx64 "\n", pc.data, BRANCH_TARGET);
}
