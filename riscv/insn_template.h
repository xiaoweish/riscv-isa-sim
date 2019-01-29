// See LICENSE for license details.

#include "arith.h"
#include "mmu.h"
#include "softfloat.h"
#include "internals.h"
#include "specialize.h"
#include "tracer.h"
#include <assert.h>

#define INST_DIV_CYCLES 30
#define INST_DIVU_CYCLES 30
#define INST_DIVUW_CYCLES 30
#define INST_DIVW_CYCLES 30
#define INST_MUL_CYCLES 10
#define INST_MULH_CYCLES 10
#define INST_MULHSU_CYCLES 10
#define INST_MULHU_CYCLES 10
#define INST_MULW_CYCLES 10
