reg_t tmp = npc;
word_t tagctrl, tag;
get_tagctrl(tagctrl);
word_t jmp_check = get_field(tagctrl, TMASK_JMP_CHECK);
if ((jmp_check != 0) && ((RS1_TAG.data & jmp_check) == 0)) {
   throw trap_tag_check_failed();
}
tag = get_field(tagctrl, TMASK_CFLOW_INDIR_TGT);
word_t jmp_target = (RS1.data + insn.i_imm()) & ~word_t(1);
set_pc(jmp_target);
npc.tag = tag;
WRITE_RD(tmp);
tag = get_field(tagctrl, TMASK_JMP_PROP);
WRITE_RD_TAG(tag);

