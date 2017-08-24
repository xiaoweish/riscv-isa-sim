reg_t tmp = npc;
word_t tagctrl, tag;
get_tagctrl(tagctrl);
tag = get_field(tagctrl, TMASK_CFLOW_DIR_TGT);
set_pc(JUMP_TARGET);
npc.tag = tag;
WRITE_RD(tmp);
tag = get_field(tagctrl, TMASK_JMP_PROP);
WRITE_RD_TAG(tag);
if (STATE.prv == PRV_U)
   fprintf(stderr, "pc: 0x%016" PRIx64 " target: 0x%016" PRIx64 "\n", pc.data, JUMP_TARGET);
