bool write = insn.rs1() != 0;
int csr = validate_csr(insn.csr(), write);
if (csr == CSR_MNXTI) {
  p->CLIC.update_clic_nint();
  if (write) {
    reg_t new_mstatus = (p->get_csr(CSR_MSTATUS, insn, write) | (RS1 & (reg_t)0x1F));
    p->put_csr(CSR_MSTATUS, new_mstatus);
  }
  if ((p->CLIC.clic_npriv == PRV_M) &&
      (p->CLIC.clic_nlevel > get_field(RS1,MCAUSE_MPIL)) &&
      (p->CLIC.clic_nlevel > get_field(p->get_csr(CSR_MINTTHRESH,insn,false), MINTTHRESH_TH)) &&
      (p->CLIC.clicintattr[p->CLIC.clic_id].shv == 0)) {
    if (((RS1 & (reg_t)0x1F) != 0) && write) {
      reg_t new_mintstatus = p->get_csr(CSR_MINTSTATUS,insn,false);
      new_mintstatus = set_field(new_mintstatus, MINTSTATUS_MIL, p->CLIC.clic_nlevel);
      p->put_csr(CSR_MINTSTATUS, new_mintstatus);
      reg_t new_mcause = p->get_csr(CSR_MCAUSE, insn, false);
      new_mcause = set_field(new_mcause, MCAUSE_EXCCODE, p->CLIC.clic_id);
      if (p->get_xlen() > 32) {
        new_mcause = set_field(new_mcause,MCAUSE64_INT,1);
      } else {
        new_mcause = set_field(new_mcause,MCAUSE_INT,1);
      }
      p->put_csr(CSR_MCAUSE, new_mcause);
      if ((p->CLIC.clicintattr[p->CLIC.clic_id].trig & 1) == 1) {
        p->CLIC.clicintip[p->CLIC.clic_id] = 0;
      }
    }
    reg_t new_rd = (p->get_csr(CSR_MTVT, insn, false)) & ~(reg_t)0x3F;
    new_rd = new_rd + p->get_xlen()/8 * p->CLIC.clic_id;
    WRITE_RD(sext_xlen(new_rd));
  } else {
    WRITE_RD(sext_xlen(0));
  }
} else if (csr == CSR_SNXTI) {
  p->CLIC.update_clic_nint();
  if (write) {
    reg_t new_sstatus = (p->get_csr(CSR_SSTATUS, insn, write) | (RS1 & (reg_t)0x1F));
    p->put_csr(CSR_SSTATUS, new_sstatus);
  }
  if ((p->CLIC.clic_npriv == PRV_S) &&
      (p->CLIC.clic_nlevel > get_field(RS1,SCAUSE_SPIL)) &&
      (p->CLIC.clic_nlevel > get_field(p->get_csr(CSR_SINTTHRESH,insn,false), SINTTHRESH_TH)) &&
      (p->CLIC.clicintattr[p->CLIC.clic_id].shv == 0)) {
    if (((RS1 & (reg_t)0x1F) != 0) && write) {
      reg_t new_sintstatus = p->get_csr(CSR_SINTSTATUS,insn,false);
      new_sintstatus = set_field(new_sintstatus, SINTSTATUS_SIL, p->CLIC.clic_nlevel);
      p->put_csr(CSR_SINTSTATUS, new_sintstatus);
      reg_t new_scause = p->get_csr(CSR_SCAUSE, insn, false);
      new_scause = set_field(new_scause, SCAUSE_EXCCODE, p->CLIC.clic_id);
      if (p->get_xlen() > 32) {
        new_scause = set_field(new_scause,SCAUSE64_INT,1);
      } else {
        new_scause = set_field(new_scause,SCAUSE_INT,1);
      }
      p->put_csr(CSR_SCAUSE, new_scause);
      if ((p->CLIC.clicintattr[p->CLIC.clic_id].trig & 1) == 1) {
        p->CLIC.clicintip[p->CLIC.clic_id] = 0;
      }
    }
    reg_t new_rd = (p->get_csr(CSR_STVT, insn, false)) & ~(reg_t)0x3F;
    new_rd = new_rd + p->get_xlen()/8 * p->CLIC.clic_id;
    WRITE_RD(sext_xlen(new_rd));
  } else {
    WRITE_RD(sext_xlen(0));
  }
} else {
  reg_t old = p->get_csr(csr, insn, write);
  if (write) {
    p->put_csr(csr, old | RS1);
  }
  WRITE_RD(sext_xlen(old));
}
serialize();
