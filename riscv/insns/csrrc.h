int csr = validate_csr(insn.csr(), true);
word_t old_tag;
word_t old = p->get_csr(csr, &old_tag);
p->set_csr(csr, old & ~RS1.data, RS1_TAG.data);
WRITE_RD(reg_t(sext_xlen(old)));
switch (csr) {
   case CSR_MEPC:
   case CSR_MSCRATCH:
   case CSR_MTVEC:
   case CSR_SEPC:
   case CSR_SSCRATCH:
   case CSR_STVEC:
   case CSR_STAGCTRL_SCRATCH:
   case CSR_MTAGCTRL_SCRATCH:
      WRITE_RD_TAG(old_tag);
      break;
   default:
      break;
}

