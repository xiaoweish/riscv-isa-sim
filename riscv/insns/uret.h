require_extension('U');
require_extension('N');
if (p->extension_enabled(EXT_SMCLICSHV) && (STATE.ucause->read() & UCAUSE_INHV)) {
  xlate_flags_t my_xlate_flags = {0,0,0,0};
  reg_t new_uepc = p->get_mmu()->load<uint32_t>(p->get_state()->uepc->read(),my_xlate_flags);
  set_pc_and_serialize(new_uepc);
} else {
  set_pc_and_serialize(p->get_state()->uepc->read());
}
reg_t s = STATE.ustatus->read();
s = set_field(s, MSTATUS_UIE, get_field(s, MSTATUS_UPIE));
s = set_field(s, MSTATUS_UPIE, 1);
p->put_csr(CSR_USTATUS, s);
p->set_privilege(PRV_U, false);
