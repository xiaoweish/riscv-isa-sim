#include <sys/time.h>
#include <sstream>
#include "devices.h"
#include "processor.h"
#include "simif.h"
#include "sim.h"
#include "dts.h"

clic_t::clic_t(const simif_t* sim, uint64_t freq_hz, bool real_time)
  : sim(sim), freq_hz(freq_hz), real_time(real_time), mtime(0)
{
  struct timeval base;

  gettimeofday(&base, NULL);

  real_time_ref_secs = base.tv_sec;
  real_time_ref_usecs = base.tv_usec;
  tick(0);
}

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

bool clic_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
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
      switch (byte_offset)
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
      return true;
    }
  } else {
    return false;
  }
  return true;
}

bool clic_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
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
      switch (byte_offset)
      {
      case 0:
        clicintip[index] = bytes[idx];
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
      return true;
    }
  } else {
    return false;
  }
  return true;
}

void clic_t::tick(reg_t rtc_ticks)
{
  if (real_time) {
   struct timeval now;
   uint64_t diff_usecs;
 
   gettimeofday(&now, NULL);
   diff_usecs = ((now.tv_sec - real_time_ref_secs) * 1000000) + (now.tv_usec - real_time_ref_usecs);
   mtime = diff_usecs * freq_hz / 1000000;
  } else {
    mtime += rtc_ticks;
  }

  for (const auto& [hart_id, hart] : sim->get_harts()) {
    hart->state.time->sync(mtime);
    hart->state.mip->backdoor_write_with_mask(MIP_MTIP, mtime >= mtimecmp[hart_id] ? MIP_MTIP : 0);
  }
}

clic_t* clic_parse_from_fdt(const void* fdt, const sim_t* sim, reg_t* base,
    const std::vector<std::string>& sargs) {
  if (fdt_parse_clic(fdt, base, "riscv,clic0") == 0 || fdt_parse_clic(fdt, base, "sifive,clic0") == 0)
    return new clic_t(sim,
                       sim->CPU_HZ / sim->INSNS_PER_RTC_TICK,
                       sim->get_cfg().real_time_clic);
  else
    return nullptr;
}

std::string clic_generate_dts(const sim_t* sim) {
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
