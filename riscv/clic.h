#ifndef _CLIC_H
#define _CLIC_H
#include <iostream>
#include <iomanip>
#include "abstract_device.h"
#include "decode.h"
#include "csrs.h"
#include "platform.h"
#include "trap.h"
#include "decode_macros.h"

class processor_t;
class simif_t;

class clic_t : public abstract_device_t
{
public:
    processor_t* p;
    simif_t* sim;
    void reset();
    bool load(reg_t addr, size_t len, uint8_t* bytes) override;
    bool store(reg_t addr, size_t len, const uint8_t* bytes) override;
    void take_clic_interrupt();
    void take_clic_trap(trap_t& t, reg_t epc);
    clic_t(/* args */);
    ~clic_t();
private:
  union clicinttrig_union
  {
    struct {
        unsigned int interrupt_number : 13;
        unsigned int reserved_warl : 18;
        unsigned int enable : 1;
    };
    unsigned int all;
  };

  typedef union clicinttrig_union CLICINTTRIG_UNION_T;

  union clicintattr_union
  {
    struct {
      uint8_t reserved_warl : 1;
      uint8_t trig : 2;
      uint8_t reserved_wpri : 3;
      uint8_t mode : 2;
    };
    uint8_t all;
  };

  typedef union clicintattr_union CLICINTATTR_UNION_T;
  
  CLICINTTRIG_UNION_T clicinttrig[CLIC_NUM_TRIGGER]   {0};
  uint8_t             clicintip[CLIC_NUM_INTERRUPT]   {0};
  uint8_t             clicintie[CLIC_NUM_INTERRUPT]   {0};
  CLICINTATTR_UNION_T clicintattr[CLIC_NUM_INTERRUPT] {0};
  uint8_t  clicintctl[CLIC_NUM_INTERRUPT]  {0};

  reg_t curr_priv;  // current privilege mode
  bool curr_ie;    // current interrupt enable ie = (xstatus.xie & clicintie[i])
  reg_t curr_level; // current interrupt level  L = max(xintstatus.xil, xintthresh.th)
  reg_t prev_priv;  // previous privilege mode
  bool prev_ie;    // previous interrupt enable ie = (xstatus.xie & clicintie[i])
  reg_t prev_level; // previous interrupt level  L = max(xintstatus.xil, xintthresh.th)
  
  // highest ranked interrupt currently present in CLIC
  reg_t clic_npriv;
  reg_t clic_nlevel;
  reg_t clic_id;

  bool  clic_vrtcl_or_hrzntl_int;  // clic vertical (1) or horizontal interrupt (0)

};

#endif
