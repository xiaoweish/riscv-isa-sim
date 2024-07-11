// See LICENSE for license details.

#ifndef _RISCV_TRAP_H
#define _RISCV_TRAP_H

#include "decode.h"
#include "encoding.h"
#include <string>

struct state_t;

class trap_debug_mode
{
  /* Used to enter debug mode, which isn't quite a normal trap. */
};

class trap_t
{
 public:
  trap_t(reg_t which) : which(which) {}
  virtual bool has_gva() { return false; }
  virtual bool has_tval() { return false; }
  virtual reg_t get_tval(bool save_ii_bits = false, bool zero_on_ebreak = false, bool zero_on_addr_misalign = false, bool custom_uncanonical = false, reg_t satp_read = 0){return 0;}
  virtual bool has_tval2() { return false; }
  virtual reg_t get_tval2() { return 0; }
  virtual bool has_tinst() { return false; }
  virtual reg_t get_tinst() { return 0; }
  reg_t cause() const { return which; }

  virtual std::string name()
  {
    const uint8_t code = uint8_t(which);
    const bool is_interrupt = code != which;
    return (is_interrupt ? "interrupt #" : "trap #") + std::to_string(code);
  }

  virtual ~trap_t() = default;

 private:
  reg_t which;
};

class insn_trap_t : public trap_t
{
 public:
  insn_trap_t(reg_t which, bool gva, reg_t tval)
    : trap_t(which), gva(gva), tval(tval) {}
  bool has_gva() override { return gva; }
  bool has_tval() override { return true; }
  reg_t get_tval(bool save_ii_bits = false, bool zero_on_ebreak = false, bool zero_on_addr_misalign = false, bool custom_uncanonical = false, reg_t satp_read = 0)override
  {
    if ((cause() == CAUSE_ILLEGAL_INSTRUCTION) && (save_ii_bits == false))
      return 0;
    if ((cause() == CAUSE_BREAKPOINT) && (zero_on_ebreak == true))
      return 0;
    return tval;
  }
 private:
  bool gva;
  reg_t tval;
};

class mem_trap_t : public trap_t
{
 public:
  mem_trap_t(reg_t which, bool gva, reg_t tval, reg_t tval2, reg_t tinst)
    : trap_t(which), gva(gva), tval(tval), tval2(tval2), tinst(tinst) {}
  bool has_gva() override { return gva; }
  uint64_t custom_mtval_unc(reg_t value, int svBits)
  {
    uint64_t mask = (1ULL << svBits) - 1;
    uint64_t final_val = value & mask;
    uint64_t extractBit = (final_val >> (svBits-1)) & 0x01;
    for (uint64_t i = svBits; i < 64; i++) {
      final_val = final_val | (extractBit << i);
    }
    final_val = final_val ^ (1ULL << 63);
    return final_val;
  }
  bool has_tval() override { return true; }
  reg_t get_tval(bool save_ii_bits = false, bool zero_on_ebreak = false, bool zero_on_addr_misalign = false, bool custom_uncanonical = false, reg_t satp_read = 0)override
  {
    if ((cause() == CAUSE_MISALIGNED_FETCH) && (zero_on_addr_misalign == true))
      return 0;
    if (((cause() == CAUSE_STORE_PAGE_FAULT) || (cause() == CAUSE_LOAD_PAGE_FAULT)) && (custom_uncanonical == true))
    {
        satp_read = get_field(satp_read, SATP64_MODE);
        if(satp_read == SATP_MODE_SV39){
            if ((tval >= 0x0000004000000000) && (tval <= 0xFFFFFFBFFFFFFFFF)) {
              return (custom_mtval_unc(tval, 39));}
        }
        else if(satp_read == SATP_MODE_SV48){
            if ((tval >= 0x0000800000000000) && (tval <= 0xFFFF7FFFFFFFFFFF)) {
              return (custom_mtval_unc(tval, 48));}
        }
        else if(satp_read == SATP_MODE_SV57){
            if ((tval >= 0x0100000000000000) && (tval <= 0xFEFFFFFFFFFFFFFF)) {
              return (custom_mtval_unc(tval, 57));}
        }
        else
          return tval;
    }
    return tval;
  }
  bool has_tval2() override { return true; }
  reg_t get_tval2() override { return tval2; }
  bool has_tinst() override { return true; }
  reg_t get_tinst() override { return tinst; }
 private:
  bool gva;
  reg_t tval, tval2, tinst;
};

#define DECLARE_TRAP(n, x) class trap_##x : public trap_t { \
 public: \
  trap_##x() : trap_t(n) {} \
  std::string name() { return "trap_"#x; } \
};

#define DECLARE_INST_TRAP(n, x) class trap_##x : public insn_trap_t { \
 public: \
  trap_##x(reg_t tval) : insn_trap_t(n, /*gva*/false, tval) {} \
  std::string name() { return "trap_"#x; } \
};

#define DECLARE_INST_WITH_GVA_TRAP(n, x) class trap_##x : public insn_trap_t {  \
 public: \
  trap_##x(bool gva, reg_t tval) : insn_trap_t(n, gva, tval) {} \
  std::string name() { return "trap_"#x; } \
};

#define DECLARE_MEM_TRAP(n, x) class trap_##x : public mem_trap_t { \
 public: \
  trap_##x(bool gva, reg_t tval, reg_t tval2, reg_t tinst) : mem_trap_t(n, gva, tval, tval2, tinst) {} \
  std::string name() { return "trap_"#x; } \
};

#define DECLARE_MEM_GVA_TRAP(n, x) class trap_##x : public mem_trap_t { \
 public: \
  trap_##x(reg_t tval, reg_t tval2, reg_t tinst) : mem_trap_t(n, true, tval, tval2, tinst) {} \
  std::string name() { return "trap_"#x; } \
};

DECLARE_MEM_TRAP(CAUSE_MISALIGNED_FETCH, instruction_address_misaligned)
DECLARE_MEM_TRAP(CAUSE_FETCH_ACCESS, instruction_access_fault)
DECLARE_INST_TRAP(CAUSE_ILLEGAL_INSTRUCTION, illegal_instruction)
DECLARE_INST_WITH_GVA_TRAP(CAUSE_BREAKPOINT, breakpoint)
DECLARE_MEM_TRAP(CAUSE_MISALIGNED_LOAD, load_address_misaligned)
DECLARE_MEM_TRAP(CAUSE_MISALIGNED_STORE, store_address_misaligned)
DECLARE_MEM_TRAP(CAUSE_LOAD_ACCESS, load_access_fault)
DECLARE_MEM_TRAP(CAUSE_STORE_ACCESS, store_access_fault)
DECLARE_TRAP(CAUSE_USER_ECALL, user_ecall)
DECLARE_TRAP(CAUSE_SUPERVISOR_ECALL, supervisor_ecall)
DECLARE_TRAP(CAUSE_VIRTUAL_SUPERVISOR_ECALL, virtual_supervisor_ecall)
DECLARE_TRAP(CAUSE_MACHINE_ECALL, machine_ecall)
DECLARE_MEM_TRAP(CAUSE_FETCH_PAGE_FAULT, instruction_page_fault)
DECLARE_MEM_TRAP(CAUSE_LOAD_PAGE_FAULT, load_page_fault)
DECLARE_MEM_TRAP(CAUSE_STORE_PAGE_FAULT, store_page_fault)
DECLARE_MEM_GVA_TRAP(CAUSE_FETCH_GUEST_PAGE_FAULT, instruction_guest_page_fault)
DECLARE_MEM_GVA_TRAP(CAUSE_LOAD_GUEST_PAGE_FAULT, load_guest_page_fault)
DECLARE_INST_TRAP(CAUSE_VIRTUAL_INSTRUCTION, virtual_instruction)
DECLARE_MEM_GVA_TRAP(CAUSE_STORE_GUEST_PAGE_FAULT, store_guest_page_fault)
DECLARE_INST_TRAP(CAUSE_SOFTWARE_CHECK_FAULT, software_check)

#endif
