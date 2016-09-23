// See LICENSE for license details.

#include "tagcachesim.h"
#include <cassert>
#include <cstring>
#include <cassert>

// ----------------- tag cache base class ----------------------- //

static uint8_t *empty_block = NULL;

tag_cache_sim_t::tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, sim_t* sim)
  : cache_sim_t(sets, ways, linesz, name), tagsz(tagsz), sim(sim)
{
  init();
}

tag_cache_sim_t::tag_cache_sim_t(const tag_cache_sim_t& rhs)
  : cache_sim_t(rhs), tagsz(rhs.tagsz), sim(rhs.sim)
{
  init();
  memcpy(datas, rhs.datas, sets*ways*linesz*sizeof(uint8_t));
}

tag_cache_sim_t::~tag_cache_sim_t() {
  delete [] datas;
}

void tag_cache_sim_t::init() {
  datas = new uint8_t[sets*ways*linesz]();
  if(empty_block == NULL) empty_block = new uint8_t[linesz]();
}

uint64_t* tag_cache_sim_t::check_tag(uint64_t addr, size_t &row) {
  size_t idx = (addr >> idx_shift) & (sets-1);
  size_t tag = (addr >> idx_shift) | VALID;

  for (size_t i = 0; i < ways; i++)
    if (tag == (tags[idx*ways + i] & ~(DIRTY|TAGFLAG))) {
      row = idx*ways + i;
      return &tags[idx*ways + i];
    }
  return NULL;
}

uint64_t* tag_cache_sim_t::victimize(uint64_t addr, size_t &row) {
  size_t idx = (addr >> idx_shift) & (sets-1);
  size_t way = lfsr.next() % ways;
  row = idx*ways + way;
  return &tags[row];
}

void tag_cache_sim_t::refill(uint64_t addr, size_t row) {
  memcpy(datas + row*linesz, sim->addr_to_mem(addr), linesz);
  if(!memcmp(datas + row*linesz, empty_block, linesz)) {
    tags[row] = (addr >> idx_shift) | VALID;
  } else {
    tags[row] = (addr >> idx_shift) | VALID | TAGFLAG;
  }
}

void tag_cache_sim_t::writeback(size_t row) {
  tags[row] = 0;
  if((tags[row] & (VALID|DIRTY)) != (VALID|DIRTY)) return;
  if(writeback_avoid(row)) return;
  //uint64_t addr = tags[row] << idx_shift;
  //memcpy(sim->addr_to_mem(addr), datas + row*linesz, linesz);
}

void tag_cache_sim_t::verify(size_t row) {
  uint64_t addr = tags[row] << idx_shift;
  assert(!memcmp(sim->addr_to_mem(addr), datas+row*linesz, linesz));
}

uint64_t tag_cache_sim_t::read(uint64_t addr, uint64_t &data, uint8_t fetch) {
  size_t row;
  uint64_t *tag = check_tag(addr, row);
  if(tag == NULL) {
    if(!fetch) return 0;
    tag = victimize(addr, row);
    writeback(row);
    refill(addr, row);
  }
  if((*tag) & TAGFLAG) {        // read data
    data = *(uint64_t *)(datas + data_row_addr(addr, row));
  } else {                      // cached but empty
    data = 0;
  }
  return *tag;
}

uint64_t tag_cache_sim_t::write(uint64_t addr, uint64_t data, uint64_t mask) {
  uint64_t rdata, wdata;
  size_t row;
  uint64_t *tag;
  read(addr, rdata, 1);         // force fetch+read
  tag = check_tag(addr, row);   // get tag and row
  wdata = (rdata & ~mask) | (data & mask); // get the udpate data
  if(rdata != wdata) {                     // update
    *(uint64_t *)(datas + data_row_addr(addr, row)) = wdata;
    *tag |= DIRTY;
    if(!memcmp(datas + row*linesz, empty_block, linesz)) {
      *tag &= ~TAGFLAG;
    } else {
      *tag |= TAGFLAG;
    }
  }
  verify(row);
  return *tag;
}

uint64_t tag_cache_sim_t::create(uint64_t addr, uint64_t data, uint64_t mask) {
  uint64_t rdata, wdata;
  size_t row;
  uint64_t *tag = check_tag(addr, row);
  if(tag == NULL) {
    tag = victimize(addr, row);
    writeback(row);
    memcpy(datas + row*linesz, empty_block, linesz); // empty block
    *tag |= (addr >> idx_shift) | VALID | DIRTY;
    rdata = 0;
  } else {
    rdata = *(uint64_t *)(datas + data_row_addr(addr, row));
  }
  wdata = (rdata & ~mask) | (data & mask); // get the udpate data
  if(rdata != wdata) {                     // update
    *(uint64_t *)(datas + data_row_addr(addr, row)) = wdata;
    *tag |= DIRTY;
    if(!memcmp(datas + row*linesz, empty_block, linesz)) {
      *tag &= ~TAGFLAG;
    } else {
      *tag |= TAGFLAG;
    }
  }
  verify(row);
  return *tag;
}

// ----------------- separate tag table class ------------------- //

tag_table_sim_t::tag_table_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, sim_t* sim)
  : tag_cache_sim_t(sets, ways, linesz, tagsz, name, sim)
{
}

// ----------------- separate tag map class --------------------- //
tag_map_sim_t::tag_map_sim_t(size_t sets, size_t ways, size_t linesz, const char* name, sim_t* sim)
  : tag_cache_sim_t(sets, ways, linesz, 0, name, sim)
{
}

// ----------------- unified tag cache class -------------------- //
unified_tag_cache_sim_t::unified_tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, sim_t* sim)
  : tag_cache_sim_t(sets, ways, linesz, tagsz, name, sim)
{
}

