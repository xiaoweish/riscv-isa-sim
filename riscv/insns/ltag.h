require_rv64;
WRITE_RD(MMU.load_tag(RS1.data + insn.i_imm()));
