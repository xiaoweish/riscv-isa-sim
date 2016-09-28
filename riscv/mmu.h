// See LICENSE for license details.

#ifndef _RISCV_MMU_H
#define _RISCV_MMU_H

#include "decode.h"
#include "trap.h"
#include "common.h"
#include "config.h"
#include "sim.h"
#include "processor.h"
#include "memtracer.h"
#include <stdlib.h>
#include <vector>

// virtual memory configuration
#define PGSHIFT 12
const word_t PGSIZE = 1 << PGSHIFT;

struct insn_fetch_t
{
  insn_func_t func;
  insn_t insn;
};

struct icache_entry_t {
  word_t tag;
  word_t pad;
  insn_fetch_t data;
};

// this class implements a processor's port into the virtual memory system.
// an MMU and instruction cache are maintained for simulator performance.
class mmu_t
{
public:
  mmu_t(sim_t* sim, processor_t* proc);
  ~mmu_t();

  // template for functions that load an aligned value from memory
#define load_func(type, signed)                                     \
    reg_t load_##type(word_t addr) __attribute__((always_inline)) { \
      type##_t res; \
      if (addr & (sizeof(type##_t)-1)) \
        throw trap_load_address_misaligned(addr); \
      word_t vpn = addr >> PGSHIFT; \
      if (likely(tlb_load_tag[vpn % TLB_ENTRIES] == vpn)) {     \
        res = *(type##_t*)(tlb_data[vpn % TLB_ENTRIES] + addr); \
      } else {                                                  \
        load_slow_path(addr, sizeof(type##_t), (uint8_t*)&res); \
      }                                                         \
      return reg_t(signed ? (sword_t)(res) : (word_t)(res));    \
    }

  // load value from memory at aligned address; zero extend to register width
  load_func(uint8, 0)
  load_func(uint16, 0)
  load_func(uint32, 0)
  load_func(uint64, 0)

  // load value from memory at aligned address; sign extend to register width
  load_func(int8, 1)
  load_func(int16, 1)
  load_func(int32, 1)
  load_func(int64, 1)

  // template for functions that store an aligned value to memory
  #define store_func(type) \
    void store_##type(word_t addr, reg_t val) { \
      if (addr & (sizeof(type##_t)-1)) \
        throw trap_store_address_misaligned(addr); \
      word_t vpn = addr >> PGSHIFT; \
      if (likely(tlb_store_tag[vpn % TLB_ENTRIES] == vpn)) \
        *(type##_t*)(tlb_data[vpn % TLB_ENTRIES] + addr) = (type##_t)(val.data); \
      else \
        store_slow_path(addr, sizeof(type##_t), (const uint8_t*)&(val.data));  \
    }

  // store value to memory at aligned address
  store_func(uint8)
  store_func(uint16)
  store_func(uint32)
  store_func(uint64)

  static const word_t ICACHE_ENTRIES = 1024;

  inline size_t icache_index(word_t addr)
  {
    return (addr / PC_ALIGN) % ICACHE_ENTRIES;
  }

  inline icache_entry_t* refill_icache(word_t addr, icache_entry_t* entry)
  {
    const uint16_t* iaddr = translate_insn_addr(addr);
    word_t insn_m = *iaddr;
    int length = insn_length(insn_m);

    if (likely(length == 4)) {
      insn_m |= (word_t)*(const int16_t*)translate_insn_addr(addr + 2) << 16;
    } else if (length == 2) {
      insn_m = (int16_t)insn_m;
    } else if (length == 6) {
      insn_m |= (word_t)*(const int16_t*)translate_insn_addr(addr + 4) << 32;
      insn_m |= (word_t)*(const uint16_t*)translate_insn_addr(addr + 2) << 16;
    } else {
    static_assert(sizeof(insn_bits_t::data) == 8, "insn_bits_t must be uint64_t");
      insn_m |= (word_t)*(const int16_t*)translate_insn_addr(addr + 6) << 48;
      insn_m |= (word_t)*(const uint16_t*)translate_insn_addr(addr + 4) << 32;
      insn_m |= (word_t)*(const uint16_t*)translate_insn_addr(addr + 2) << 16;
    }

    insn_t insn = insn_t(insn_bits_t(insn_m));
    insn_fetch_t fetch = {proc->decode_insn(insn), insn};
    entry->tag = addr;
    entry->data = fetch;

    word_t paddr = sim->mem_to_addr((char*)iaddr);
    if (sim->cacheable(paddr) && tracer.interested_in_range(paddr, paddr + 1, FETCH)) {
      entry->tag = -1;
      tracer.trace(paddr, length, FETCH);
    }
    return entry;
  }

  inline icache_entry_t* access_icache(word_t addr)
  {
    icache_entry_t* entry = &icache[icache_index(addr)];
    if (likely(entry->tag == addr))
      return entry;
    return refill_icache(addr, entry);
  }

  inline insn_fetch_t load_insn(word_t addr)
  {
    return access_icache(addr)->data;
  }

  void flush_tlb();
  void flush_icache();

  void register_memtracer(memtracer_t*);

private:
  sim_t* sim;
  processor_t* proc;
  memtracer_list_t tracer;
  uint16_t fetch_temp;

  // implement an instruction cache for simulator performance
  icache_entry_t icache[ICACHE_ENTRIES];

  // implement a TLB for simulator performance
  static const word_t TLB_ENTRIES = 256;
  char* tlb_data[TLB_ENTRIES];
  word_t tlb_insn_tag[TLB_ENTRIES];
  word_t tlb_load_tag[TLB_ENTRIES];
  word_t tlb_store_tag[TLB_ENTRIES];

  // finish translation on a TLB miss and upate the TLB
  void refill_tlb(word_t vaddr, word_t paddr, access_type type);

  // perform a page table walk for a given VA; set referenced/dirty bits
  word_t walk(word_t addr, access_type type, bool supervisor, bool pum);

  // handle uncommon cases: TLB misses, page faults, MMIO
  const uint16_t* fetch_slow_path(word_t addr);
  void load_slow_path(word_t addr, word_t len, uint8_t* bytes);
  void store_slow_path(word_t addr, word_t len, const uint8_t* bytes);
  word_t translate(word_t addr, access_type type);

  // ITLB lookup
  const uint16_t* translate_insn_addr(word_t addr) __attribute__((always_inline)) {
    word_t vpn = addr >> PGSHIFT;
    if (likely(tlb_insn_tag[vpn % TLB_ENTRIES] == vpn))
      return (uint16_t*)(tlb_data[vpn % TLB_ENTRIES] + addr);
    return fetch_slow_path(addr);
  }
  
  friend class processor_t;
};

#endif
