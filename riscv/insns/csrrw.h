int csr = validate_csr(insn.csr(), true);
if (csr == CSR_USCRATCHCSWL) {
  if (((p->get_csr(CSR_UCAUSE, insn, false, true) & UCAUSE_UPIL) == 0) !=
      ((p->get_csr(CSR_UINTSTATUS, insn, false, true) & UINTSTATUS_UIL) == 0))
  {
    reg_t old = p->get_csr(CSR_USCRATCH, insn, true);
    p->put_csr(CSR_USCRATCH, RS1);
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
