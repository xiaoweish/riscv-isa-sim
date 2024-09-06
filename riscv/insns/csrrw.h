int csr = validate_csr(insn.csr(), true);
if (csr == CSR_MSCRATCHCSW) {
  if ((p->get_state()->prv) == ((p->get_csr(CSR_MSTATUS, insn, false, true) & MSTATUS_MPP) >> MSTATUS_MPP_LSB)) {
    WRITE_RD(sext_xlen(RS1));
  } else {
    reg_t old = p->get_csr(CSR_MSCRATCH, insn, true);
    p->put_csr(CSR_MSCRATCH, RS1);
    WRITE_RD(sext_xlen(old));
  }
} else if (csr == CSR_MSCRATCHCSWL) {
  if (((p->get_csr(CSR_MCAUSE, insn, false, true) & MCAUSE_MPIL) == 0) !=
      ((p->get_csr(CSR_MINTSTATUS, insn, false, true) & MINTSTATUS_MIL) == 0))
  {
    reg_t old = p->get_csr(CSR_MSCRATCH, insn, true);
    p->put_csr(CSR_MSCRATCH, RS1);
    WRITE_RD(sext_xlen(old));
  }
  else
  {
    WRITE_RD(sext_xlen(RS1));
  }
} else {
  reg_t old = p->get_csr(csr, insn, true);
  p->put_csr(csr, RS1);
  WRITE_RD(sext_xlen(old));
}
serialize();
