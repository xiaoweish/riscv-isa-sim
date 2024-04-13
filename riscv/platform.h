// See LICENSE for license details.
#ifndef _RISCV_PLATFORM_H
#define _RISCV_PLATFORM_H

#define DEFAULT_KERNEL_BOOTARGS "console=ttyS0 earlycon"
#define DEFAULT_RSTVEC     0x00001000
#define CLINT_BASE         0x02000000
#define CLINT_SIZE         0x000c0000
#define PLIC_BASE          0x0c000000
#define PLIC_SIZE          0x01000000
#define PLIC_NDEV          31
#define PLIC_PRIO_BITS     4
#define NS16550_BASE       0x10000000
#define NS16550_SIZE       0x100
#define NS16550_REG_SHIFT  0
#define NS16550_REG_IO_WIDTH 1
#define NS16550_INTERRUPT_ID 1
#define EXT_IO_BASE        0x40000000
#define DRAM_BASE          0x80000000

// #define MCLIC_BASE 0x0d000000
#define MCLIC_BASE 0x0d000000
#define MCLIC_SIZE 0x04000000

#define CLIC_NVBITS 0         // Specifying Support for smclicshv - Selective Interrupt Hardware Vectoring Extension: 0 = not implemented, 1 = implemented
#define CLIC_NUM_INTERRUPT 64 // 2-4096 that specifies the actual number of maximum interrupt inputs supported in this implementation. MSIP, MTIP are always included.
#define CLIC_VERSION 0x01     // 8-bit  parameter  specifies  the  implementation  version  of  CLIC.  The  upper  4-bit  specifies the architecture version, and the lower 4-bit specifies the implementation version
#define CLIC_INTCTLBITS 8     // specifies how many hardware bits are actually implemented in the clicintctl registers, with 0 ≤ MCLIC_INTCTLBITS ≤ 8
#define CLIC_NUM_TRIGGER 16  // specifies the number of maximum interrupt triggers supported in this implementation. Valid values are 0 to 32.

#define CLIC_ANDBASIC                           0  // 0 - CLIC only, 1 Implements CLINT mode also
#define CLIC_PRIVMODES                          1  // 1-3 Number privilege modes: 1=M, 2=M/U, 3=M/S/U
#define CLIC_LEVELS                            16  // 2-256 Number of interrupt levels including 0
#define CLIC_MAXID          CLIC_NUM_INTERRUPT-1   // 12-4095 Largest interrupt ID
#define CLIC_INTTHRESHBITS                      8  // 1-8 Number of bits implemented in {intthresh}.th
#define CLIC_CFGMBITS                           0  // 0-ceil(lg2(CLICPRIVMODES)) Number of bits implemented for cliccfg.nmbits
#define CLIC_CFGLBITS                           4  // 0-ceil(lg2(CLICLEVELS)) Number of bits implemented for cliccfg.nlbits
#define CLIC_MTVECALIGN                         6  // >= 6 Number of hardwired-zero least significant bits in mtvec address

#endif
