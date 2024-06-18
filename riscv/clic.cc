#include "clic.h"
#include "processor.h"
#include "arith.h"
#include "dts.h"
#include "sim.h"


// M-mode CLIC memory map - 12/19/2023 - version 0.9-draft
//   Offset
//   0x0000         4B          RW        reserved for smclicconfig extension
//
//// 0x0004-0x003F              reserved    ###
//
//   0x0040         4B          RW        clicinttrig[0]
//   0x0044         4B          RW        clicinttrig[1]
//   0x0048         4B          RW        clicinttrig[2]
//   ...
//   0x00B4         4B          RW        clicinttrig[29]
//   0x00B8         4B          RW        clicinttrig[30]
//   0x00BC         4B          RW        clicinttrig[31]
//
//// 0x00C0-0x07FF              reserved    ###
//// 0x0800-0x0FFF              custom      ###
//
//   0x1000+4*i     1B/input    R or RW   clicintip[i]
//   0x1001+4*i     1B/input    RW        clicintie[i]
//   0x1002+4*i     1B/input    RW        clicintattr[i]
//   0x1003+4*i     1B/input    RW        clicintctl[i]
//   ...
//   0x4FFC         1B/input    R or RW   clicintip[4095]
//   0x4FFD         1B/input    RW        clicintie[4095]
//   0x4FFE         1B/input    RW        clicintattr[4095]
//   0x4FFF         1B/input    RW        clicintctl[4095]

#define CLIC_SMCLICCONFIG_EXT_OFFSET  0X0000

#define CLIC_RESERVED1_BASE_OFFSET    0X0004
#define CLIC_RESERVED1_TOP_OFFSET     0X003F

#define CLIC_INTTRIG_ADDR_BASE_OFFSET 0X0040
#define CLIC_INTTRIG_ADDR_TOP_OFFSET  0X00BC

#define CLIC_RESEVED2_BASE_OFFSET     0X00C0
#define CLIC_RESEVED2_TOP_OFFSET      0X07FF

#define CLIC_CUSTOM_BASE_OFFSET       0X0800
#define CLIC_CUSTOM_TOP_OFFSET        0X0FFF

#define CLIC_INTTBL_ADDR_BASE_OFFSET  0X1000
#define     CLIC_INTIP_BYTE_OFFSET     0X0
#define     CLIC_INTIE_BYTE_OFFSET     0X1
#define     CLIC_INTATTR_BYTE_OFFSET   0X2
#define     CLIC_INTCTL_BYTE_OFFSET    0X3
#define CLIC_INTTBL_ADDR_TOP_OFFSET   0X4FFC

clic_t::CLICINTTRIG_UNION_T clic_t::clicinttrig[CLIC_NUM_TRIGGER]   = {0};
uint8_t                     clic_t::clicintip[CLIC_NUM_INTERRUPT]   = {0};
uint8_t                     clic_t::clicintie[CLIC_NUM_INTERRUPT]   = {0};
clic_t::CLICINTATTR_UNION_T clic_t::clicintattr[CLIC_NUM_INTERRUPT] = {0};
uint8_t                     clic_t::clicintctl[CLIC_NUM_INTERRUPT]  = {0};

bool clic_t::load(reg_t addr, size_t len, uint8_t *bytes)  {
  if (len > 8)
    return false;

  tick(0);

  if ((addr >= CLIC_SMCLICCONFIG_EXT_OFFSET) && (addr < CLIC_RESERVED1_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    for (int indx = 0; indx < len; indx++)
    {
      // FIXME add cliccfg register when extension smclicconfig is enabled
      bytes[indx] = 0;
    }
    return true;
  } else if ((addr >= CLIC_RESERVED1_BASE_OFFSET) && (addr < CLIC_INTTRIG_ADDR_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    for (int indx = 0; indx < len; indx++)
    {
      bytes[indx] = 0;
    }
    return true;
  } else if ((addr >= CLIC_INTTRIG_ADDR_BASE_OFFSET) && (addr < CLIC_RESEVED2_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    /* load from clic interrupt trigger memory mapped registers */
    int index = addr - CLIC_INTTRIG_ADDR_BASE_OFFSET;
    for (int i = 0; i < len; i++)
    {
      bytes[i] = clicinttrig[index].all >> i*8;
    }
    return true;
  } else if ((addr >= CLIC_CUSTOM_BASE_OFFSET ) && (addr < CLIC_INTTBL_ADDR_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    for (int indx = 0; indx < len; indx++)
    {
      bytes[indx] = 0;
    }
    return true;
  } else if ((addr >= CLIC_INTTBL_ADDR_BASE_OFFSET ) && (addr < (CLIC_INTTBL_ADDR_TOP_OFFSET + 4))) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    int index = ((addr & 0XFFFFFFFFFFFFFFFC) - CLIC_INTTBL_ADDR_BASE_OFFSET) / 4;
    int byte_offset = addr & 0x3;
    for (int i = byte_offset; i < byte_offset + len; i++)
    {
      switch (i)
      {
      case 0:
        /* clicintip */
        bytes[i] = clicintip[index];
        break;
      case 1:
        /* clicintie */
        bytes[i] = clicintie[index];
        break;
      case 2:
        /* clicintattr */
        bytes[i] = clicintattr[index].all;
        break;
      case 3:
        /* clicintctl */
        bytes[i] = clicintctl[index];
        break;
      default:
        return false;
        break;
      }
    }
    return true;
  } else {
    return false;
  }
  return true;
}

bool clic_t::store(reg_t addr, size_t len, const uint8_t* bytes)  {
if (len > 8) {
    return false;
  }

  tick(0);

  if ((addr >= CLIC_SMCLICCONFIG_EXT_OFFSET) && (addr < CLIC_RESERVED1_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    // FIXME add cliccfg register when extension smclicconfig is enabled
    return false;
  } else if ((addr >= CLIC_RESERVED1_BASE_OFFSET) && (addr < CLIC_INTTRIG_ADDR_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    return false;
  } else if ((addr >= CLIC_INTTRIG_ADDR_BASE_OFFSET) && (addr < CLIC_RESEVED2_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    /* store to clic interrupt trigger memory mapped registers */
    int index = addr - CLIC_INTTRIG_ADDR_BASE_OFFSET;
    for (int idx = 0; idx < len; idx++)
    {
      // assuming that address is 32 bit aligned
      clicinttrig[index].all = (clicinttrig[index].all & ~((uint32_t(0xFF) << (idx * 8)))) |
                               (uint32_t(bytes[idx]) << (idx * 8));
    }
    return true;
  } else if ((addr >= CLIC_CUSTOM_BASE_OFFSET ) && (addr < CLIC_INTTBL_ADDR_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    return false;
  } else if ((addr >= CLIC_INTTBL_ADDR_BASE_OFFSET ) && (addr < (CLIC_INTTBL_ADDR_TOP_OFFSET + 4))) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    /* store to clic interrupt table memory mapped registers */
    int index = ((addr & 0XFFFFFFFFFFFFFFFC) - CLIC_INTTBL_ADDR_BASE_OFFSET) / 4;
    int byte_offset = addr & 0x3;
    for (int idx = byte_offset; idx < len; idx++)
    {
      switch (idx)
      {
      case 0:
        clicintip[index] = bytes[idx]; // check
        break;
      case 1:
        clicintie[index] = bytes[idx];
        break;
      case 2:
        clicintattr[index].all = bytes[idx];
        break;
      case 3:
        clicintctl[index] = bytes[idx];
        break;
      default:
        break;
      }
    }
    return true;
  } else {
    return false;
  }
  return true;
}

void clic_t::update_clic_nint() {
  // perform search for higest ranked interrupt in CLIC;
  uint16_t int_selected = 0;
  clic_npriv  = PRV_U;
  clic_nlevel = 0;
  clic_id     = 0;

  for (int indx = 0; indx < CLIC_NUM_INTERRUPT; indx++) {
    if (clicintip[indx] != 0) {
        if (((clicintattr[indx].mode << 8) | clicintctl[indx]) >= int_selected)
        {
        int_selected = ((clicintattr[indx].mode << 8) | clicintctl[indx]);
        clic_npriv  = clicintattr[indx].mode;
        clic_nlevel = clicintctl[indx];
        clic_id     = indx;
        }
    }
  }


};

void clic_t::take_clic_interrupt() {
    state_t* state;
    state = p->get_state();

  // CLIC, version 0.9 draft, 03/03/2024
  //  Current  |      CLIC          |->      Current'          Previous
  //  p/ie/il  | priv   level   id  |->    p/ie/il  pc  cde   pp/il/ie epc
  //  P  ?  ?  | nP<P     ?      ?  |->    - -  -   -   -     -  -  -  -   # Interrupt ignored
  //  P  0  ?  | nP=P     ?      ?  |->    - -  -   -   -     -  -  -  -   # Interrupts disabled
  //  P  1  ?  | nP=P     0      ?  |->    - -  -   -   -     -  -  -  -   # No interrupt
  //  P  1  L  | nP=P   0<nL<=L  ?  |->    - -  -   -   -     -  -  -  -   # Interrupt ignored
  //  P  1  L  | nP=P   L<nL    id  |->    P 0  nL  V   id    P  L  1  pc  # Horizontal interrupt taken
  //  P  ?  ?  | nP>P     0      ?  |->    - -  -   -   -     -  -  -  -   # No interrupt
  //  P  e  L  | nP>P   0<nL    id  |->   nP 0  nL  V   id    P  L  e  pc  # Vertical interrupt taken

// To select an interrupt to present to the core, the CLIC hardware combines the valid bits in clicintattr.mode
// and  clicintctl  to  form  an  unsigned  integer,  then  picks  the  global  maximum  across  all  pending-and-enabled
// interrupts based on this value. Next, the smclicconfig extension defines how to split the clicintctl value into
// interrupt level and interrupt priority. Finally, the interrupt level of this selected interrupt is compared with the
// interrupt-level threshold of the associated privilege mode to determine whether it is qualified or masked by the
// threshold (and thus no interrupt is presented).

  update_clic_nint();

  if ((clic_nlevel == 0) )
  {
    return;
  }
  
  curr_priv = state->prv;
  bool xstatus_xie = false;
  curr_ie = false;
  switch (curr_priv)
  {
  case PRV_U:
    //xstatus_xie = (state->ustatus->read() & USTATUS_UIE) ? true : false;
    curr_level = (state->csrmap[CSR_MINTSTATUS]->read() & MINTSTATUS_UIL) >> MINTSTATUS_UIL_LSB;
    // if ((curr_level) < (state->csrmap[CSR_UINTTHRESH]->read()))
    // {
    //   curr_level = state->csrmap[CSR_UINTTHRESH]->read();
    // }
    return; // fixme - temp solution for non-existent U-mode
    break;

  case PRV_S:
    xstatus_xie = (state->sstatus->read() & SSTATUS_SIE) ? true : false;
    curr_level = (state->csrmap[CSR_MINTSTATUS]->read() & MINTSTATUS_SIL) >> MINTSTATUS_SIL_LSB;
    // if (curr_level < state->csrmap[CSR_SINTTHRESH]->read())
    // {
    //   curr_level = state->csrmap[CSR_SINTTHRESH]->read()
    // }
    break;

  case PRV_M:
    xstatus_xie = (state->mstatus->read() & MSTATUS_MIE) ? true : false;
    curr_level = (state->csrmap[CSR_MINTSTATUS]->read() & MINTSTATUS_MIL) >> MINTSTATUS_MIL_LSB;
    if (curr_level < state->csrmap[CSR_MINTTHRESH]->read())
    {
      curr_level = state->csrmap[CSR_MINTTHRESH]->read();
    }
    break;

  default:
    break;
  } 
  
  if (clicintie[clic_id] != 0) {
    p->in_wfi = false;
  }

  curr_ie = xstatus_xie && (clicintie[clic_id] != 0);

  if ((clic_npriv > curr_priv) && (0 < clic_nlevel) && curr_ie) // vertical interrupt
  {
    clic_vrtcl_or_hrzntl_int = true;
  }
  else if ((clic_npriv == curr_priv) && (curr_level < clic_nlevel) && curr_ie) // horizontal interrupt
  {
    clic_vrtcl_or_hrzntl_int = false;
  }
  else // no interrupt
  {
    return;
  }
  
  
 if (p->check_triggers_icount) p->TM.detect_icount_match();
 throw trap_t(((reg_t)1 << (p->isa->get_max_xlen() - 1)) | clic_id);

}

void clic_t::take_clic_trap(trap_t& t, reg_t epc) {  // fixme - Implementation for SMCLIC mode only
  unsigned max_xlen = p->isa->get_max_xlen();

  // fixme - do the preexisting "debug" and/or "state.debug_mode" mode stuff need to be copied from CLINT mode ?

  reg_t bit = t.cause();
  bool curr_virt = p->state.v;
    const reg_t interrupt_bit = (reg_t)1 << (max_xlen - 1);
  bool interrupt = (bit & interrupt_bit) != 0;

  if (interrupt) {
    bit &= ~((reg_t)1 << (max_xlen - 1));
  } else {
    // fixme - code to handle exception delegation goes here
  }

  // SMCLIC does not support vectored operation -- fixme when SMCLICSHV is implemented
  const reg_t trap_handler_address = (p->state.mtvec->read() & ~(reg_t)63);

  const reg_t rnmi_trap_handler_address = 0;
  const bool nmie = !(p->state.mnstatus && !get_field(p->state.mnstatus->read(), MNSTATUS_NMIE));
  
  p->state.pc = !nmie ? rnmi_trap_handler_address : trap_handler_address;
  
  p->state.mepc->write(epc);
  
  prev_priv = curr_priv;
  prev_ie   = curr_ie;
  prev_level = curr_level;

  reg_t cause = t.cause();
  cause = set_field(cause,MCAUSE_MPP,prev_priv);
  cause = set_field(cause,MCAUSE_MPIE,prev_ie);
  cause = set_field(cause,MCAUSE_MPIL, prev_level);
  p->state.mcause->write(cause);
  
  p->state.mtval->write(t.get_tval());
  p->state.mtval2->write(t.get_tval2());

  p->state.mtinst->write(t.get_tinst());

  reg_t s = p->state.mstatus->read();
  s = set_field(s, MSTATUS_MPIE, prev_ie);
  s = set_field(s, MSTATUS_MPP, prev_priv);
  s = set_field(s, MSTATUS_MIE, 0);
  s = set_field(s, MSTATUS_MPV, curr_virt);
  s = set_field(s, MSTATUS_GVA, t.has_gva());
  s = set_field(s, MSTATUS_MPELP, p->state.elp);
  p->state.elp = elp_t::NO_LP_EXPECTED;
  p->state.mstatus->write(s);
  if (p->state.mstatush) p->state.mstatush->write(s >> 32);  // log mstatush change
  p->set_privilege(PRV_M, false);

}

void clic_t::reset() {
  auto& csrmap = p->get_state()->csrmap;
  csrmap[CSR_MTVT]  = std::make_shared<tvt_t>(p,CSR_MTVT);
  csrmap[CSR_MNXTI] = std::make_shared<nxti_t>(p, CSR_MNXTI);
  csrmap[CSR_MINTSTATUS] = std::make_shared<intstatus_t>(p, CSR_MINTSTATUS);
  csrmap[CSR_MINTTHRESH] = std::make_shared<intthresh_t>(p, CSR_MINTTHRESH);
  csrmap[CSR_MSCRATCHCSW] = std::make_shared<scratchcsw_t>(p, CSR_MSCRATCHCSW);
  csrmap[CSR_MSCRATCHCSWL] = std::make_shared<scratchcswl_t>(p, CSR_MSCRATCHCSWL);

  reg_t mintstatus = csrmap[CSR_MINTSTATUS]->read();
  mintstatus = mintstatus & ~MINTSTATUS_MIL;
  csrmap[CSR_MINTSTATUS]->write(mintstatus);
  reg_t mcause = csrmap[CSR_MCAUSE]->read();
  mcause = mcause & ~MCAUSE_MPIL;
  csrmap[CSR_MCAUSE]->write(mcause);

  curr_priv = PRV_U;
  prev_priv = PRV_U;
  curr_level = 0;
  prev_level = 0;
  curr_ie = 0;
  prev_ie = 0;
  clic_npriv = PRV_U;
  clic_nlevel = 0;
  clic_id = 0;

}

clic_t* clic_parse_from_fdt(const void* fdt, const sim_t* sim, reg_t* base,
    const std::vector<std::string>& sargs) {
  if (fdt_parse_clic(fdt, base, "riscv,clic0") == 0 || fdt_parse_clic(fdt, base, "sifive,clic0") == 0)
    return new clic_t();
  else
    return nullptr;
}

std::string clic_generate_dts(const sim_t* sim,  const std::vector<std::string>& UNUSED sargs) {
  std::stringstream s;
  s << std::hex
    << "    clic@" << MCLIC_BASE << " {\n"
       "      compatible = \"riscv,clic0\";\n"
       "      interrupts-extended = <" << std::dec;
  for (size_t i = 0; i < sim->get_cfg().nprocs(); i++)
    s << "&CPU" << i << "_intc 3 &CPU" << i << "_intc 7 ";
  reg_t clicbs = MCLIC_BASE;
  reg_t clicsz = MCLIC_SIZE;
  s << std::hex << ">;\n"
    "      reg = <0x" << (clicbs >> 32) << " 0x" << (clicbs & (uint32_t)-1) <<
    " 0x" << (clicsz >> 32) << " 0x" << (clicsz & (uint32_t)-1) << ">;\n"
    "    };\n";
  return s.str();
}

REGISTER_DEVICE(clic, clic_parse_from_fdt, clic_generate_dts)

clic_t::clic_t(/* args */) :
  p(0),
  sim(0) {
}

clic_t::~clic_t()
{
}
