require_extension('U');
require_extension('N');
set_pc_and_serialize(p->get_state()->uepc->read());
reg_t s = STATE.ustatus->read();
s = set_field(s, MSTATUS_UIE, get_field(s, MSTATUS_UPIE));
s = set_field(s, MSTATUS_UPIE, 1);
p->put_csr(CSR_USTATUS, s);
p->set_privilege(PRV_U, false);
