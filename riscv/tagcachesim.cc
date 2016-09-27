// See LICENSE for license details.

#include "tagcachesim.h"
#include <cassert>
#include <cstring>
#include <cassert>
#include <boost/algorithm/string.hpp>

// ----------------- tag cache base class ----------------------- //

static uint8_t *empty_block = NULL;

tag_cache_sim_t::tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, uint8_t wb, const char* name, sim_t* sim)
  : cache_sim_t(sets, ways, linesz, name), tagsz(tagsz), wb_enforce(wb), sim(sim)
{
  init();
}

tag_cache_sim_t::tag_cache_sim_t(const tag_cache_sim_t& rhs)
  : cache_sim_t(rhs), tagsz(rhs.tagsz), wb_enforce(rhs.wb_enforce), sim(rhs.sim)
{
  init();
  memcpy(datas, rhs.datas, sets*ways*linesz*sizeof(uint8_t));
}

tag_cache_sim_t::~tag_cache_sim_t() {
  delete [] datas;
}

void tag_cache_sim_t::init() {
  datas = new uint8_t[sets*ways*linesz]();
  tag_map = NULL;
  tag_base = DRAM_BASE + sim->memsz - sim->memsz * tagsz / 64;
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
  memcpy(datas + row*linesz, sim->addr_to_tagmem(addr, tag_base), linesz);
  if(!memcmp(datas + row*linesz, empty_block, linesz)) {
    tags[row] = (addr >> idx_shift) | VALID;
  } else {
    tags[row] = (addr >> idx_shift) | VALID | TAGFLAG;
  }
}

void tag_cache_sim_t::writeback(size_t row) {
  if((tags[row] & (VALID|DIRTY)) != (VALID|DIRTY)) return;
  if((tags[row] & TAGFLAG) || 1) {
    uint64_t addr = tags[row] << idx_shift;
    memcpy(sim->addr_to_tagmem(addr, tag_base), datas + row*linesz, linesz);
  }
  tags[row] = 0;
}

void tag_cache_sim_t::verify(size_t row) {
  uint64_t addr = tags[row] << idx_shift;
  assert(!memcmp(sim->addr_to_mem(addr), sim->addr_to_tagmem(addr, tag_base), linesz));
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
  return (*tag) & ~DIRTY;
}

uint64_t tag_cache_sim_t::write(uint64_t addr, uint64_t data, uint64_t mask) {
  uint64_t rdata, wdata;
  size_t row;
  uint64_t *tag;
  uint8_t tagFlag_chg = 0;
  read(addr, rdata, 1);         // force fetch+read
  tag = check_tag(addr, row);   // get tag and row
  wdata = (rdata & ~mask) | (data & mask); // get the udpate data
  if(rdata != wdata) {                     // update
    *(uint64_t *)(datas + data_row_addr(addr, row)) = wdata;
    *tag |= DIRTY;
    if(!memcmp(datas + row*linesz, empty_block, linesz)) {
      tagFlag_chg = (*tag & TAGFLAG) == TAGFLAG;
      *tag &= ~TAGFLAG;
    } else {
      tagFlag_chg = (*tag & TAGFLAG) == 0;
      *tag |= TAGFLAG;
    }
  }
  verify(row);
  return tagFlag_chg ? *tag | DIRTY : *tag & ~DIRTY;
}

uint64_t tag_cache_sim_t::create(uint64_t addr, uint64_t data, uint64_t mask) {
  uint64_t rdata, wdata;
  size_t row;
  uint64_t *tag = check_tag(addr, row);
  uint8_t tagFlag_chg = 0;
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
      tagFlag_chg = 1;
      *tag |= TAGFLAG;
    }
  }
  verify(row);
  return tagFlag_chg ? *tag | DIRTY : *tag & ~DIRTY;
}

// ----------------- separate tag table class ------------------- //

tag_table_sim_t::tag_table_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, uint8_t wb, const char* name, sim_t* sim)
  : tag_cache_sim_t(sets, ways, linesz, tagsz, wb, name, sim)
{
  init();
}

tag_table_sim_t::tag_table_sim_t(const tag_table_sim_t &rhs)
  : tag_cache_sim_t(rhs)
{
  init();
}

tag_cache_sim_t* tag_table_sim_t::construct(const char* config, const char* name, sim_t* sim) {
  // sets:ways:blocksize:tagsz:wb
  std::vector<std::string> args;
  std::string config_string = std::string(config);
  boost::split(args, config_string, boost::is_any_of(":"));
  assert(args.size() == 5);

  size_t sets = atoi(args[0].c_str());
  size_t ways = atoi(args[1].c_str());
  size_t linesz = atoi(args[2].c_str());
  size_t tagsz = atoi(args[3].c_str());
  size_t wb = atoi(args[4].c_str());

  return new tag_table_sim_t(sets, ways, linesz, tagsz, wb, name, sim);
}

uint64_t tag_table_sim_t::access(uint64_t addr, size_t byte, bool store) {
  uint64_t tt_tag, tt_data = 0, tt_addr = tt_base + (addr-DRAM_BASE) / (64 / tagsz);
  uint64_t tt_wmask = (((uint64_t)1 << (tagsz*byte/8)) - 1) << (addr % (64 / tagsz)) * (64 / tagsz);
  uint64_t tt_wdata = read_mem(tt_addr);
  uint64_t tm_data = 0;

  // read map if any
  if(tag_map != NULL) tm_data = tag_map->access(tt_addr, 1, 0);

  // read tag table
  if(tm_data) {                // have tags
    tt_tag = read(tt_addr, tt_data, 1);
    if(store && (tt_wdata & tt_wmask) != (tt_data & tt_wmask)) {
      tt_tag = write(tt_addr, tt_wdata, tt_wmask);
      if((tt_tag & DIRTY) && tag_map != NULL)
        tag_map->access(tt_addr, 1, 1);
    }
  } else {                      // empty
    if(store && (tt_wdata & tt_wmask) != 0) {
      tt_tag = create(tt_addr, tt_wdata, tt_wmask);
      if(tag_map != NULL)
        tag_map->access(tt_addr, 1, 1);
    }
  }
  return 0;
}

void tag_table_sim_t::init() {
  tt_base = DRAM_BASE + memsz() - memsz() / (64 / tagsz);
}

// ----------------- separate tag map class --------------------- //
tag_map_sim_t::tag_map_sim_t(size_t sets, size_t ways, size_t linesz, uint64_t tablesz, uint64_t table_linesz, uint8_t wb, const char* name, sim_t* sim)
  : tag_cache_sim_t(sets, ways, linesz, 0, wb, name, sim), tt_size(tablesz), tt_linesz(table_linesz)
{
  init();
}

tag_map_sim_t::tag_map_sim_t(const tag_map_sim_t &rhs)
  : tag_cache_sim_t(rhs), tt_size(rhs.tt_size), tt_linesz(rhs.tt_linesz)
{
  init();
}

tag_cache_sim_t* tag_map_sim_t::construct(const char* config, const char* name, sim_t* sim) {
  // sets:ways:blocksize:tablesz:table_linesz:wb
  std::vector<std::string> args;
  std::string config_string = std::string(config);
  boost::split(args, config_string, boost::is_any_of(":"));
  assert(args.size() == 6);

  size_t sets = atoi(args[0].c_str());
  size_t ways = atoi(args[1].c_str());
  size_t linesz = atoi(args[2].c_str());
  size_t tablesz = atoi(args[3].c_str());
  size_t table_linesz = atoi(args[4].c_str());
  size_t wb = atoi(args[5].c_str());

  return new tag_map_sim_t(sets, ways, linesz, tablesz, table_linesz, wb, name, sim);
}

uint64_t tag_map_sim_t::access(uint64_t addr, size_t byte, bool store) {
  uint64_t tm_tag, tm_data = 0, tm_addr = tm_base + (addr-DRAM_BASE) / (tt_linesz * 8);
  uint64_t tm_wmask = (uint64_t)1 << (addr % (tt_linesz * 8)) * (tt_linesz * 8);
  uint64_t tm_wdata = read_mem(tm_addr);
  uint64_t th_data = 0;

  // read map if any
  if(tag_map != NULL) th_data = tag_map->access(tm_addr, 1, 0);

  // read tag table
  if(th_data) {                // have tags
    tm_tag = read(tm_addr, tm_data, 1);
    if(store && (tm_wdata & tm_wmask) != (tm_data & tm_wmask)) {
      tm_tag = write(tm_addr, tm_wdata, tm_wmask);
      if((tm_tag & DIRTY) && tag_map != NULL)
        tag_map->access(tm_addr, 1, 1);
    }
  } else {                      // empty
    if(store && (tm_wdata & tm_wmask) != 0) {
      tm_tag = create(tm_addr, tm_wdata, tm_wmask);
      if(tag_map != NULL)
        tag_map->access(tm_addr, 1, 1);
    }
  }
  return tm_data & tm_wmask;
}


void tag_map_sim_t::init() {
  tm_base = DRAM_BASE + memsz() - tt_size / (tt_linesz * 8);
}

// ----------------- unified tag cache class -------------------- //
unified_tag_cache_sim_t::unified_tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, uint8_t wb, const char* name, sim_t* sim)
  : tag_cache_sim_t(sets, ways, linesz, tagsz, wb, name, sim)
{
  init();
}

unified_tag_cache_sim_t::unified_tag_cache_sim_t( const unified_tag_cache_sim_t& rhs)
  : tag_cache_sim_t(rhs)
{
  init();
}

void unified_tag_cache_sim_t::init() {
  tt_base =  DRAM_BASE + memsz() - memsz() / (64 / tagsz);
  tm0_base = DRAM_BASE + memsz() - memsz() / (64 / tagsz) / (linesz * 8);
  tm1_base = DRAM_BASE + memsz() - memsz() / (64 / tagsz) / (linesz * 8) / (linesz * 8);
}

tag_cache_sim_t* unified_tag_cache_sim_t::construct(const char* config, const char* name, sim_t* sim) {
  // sets:ways:blocksize:tagsz:wb
  std::vector<std::string> args;
  std::string config_string = std::string(config);
  boost::split(args, config_string, boost::is_any_of(":"));
  assert(args.size() == 5);

  size_t sets = atoi(args[0].c_str());
  size_t ways = atoi(args[1].c_str());
  size_t linesz = atoi(args[2].c_str());
  size_t tagsz = atoi(args[3].c_str());
  size_t wb = atoi(args[4].c_str());

  return new unified_tag_cache_sim_t(sets, ways, linesz, tagsz, wb, name, sim);
}

uint64_t unified_tag_cache_sim_t::access(uint64_t addr, size_t bytes, bool store) {
  uint64_t tt_tag = 0, tt_data = 0, tt_addr = tt_base + (addr-DRAM_BASE) / (64 / tagsz);
  uint64_t tm0_tag = 0, tm0_data = 0, tm0_addr = tm0_base + (tt_addr-DRAM_BASE) / (linesz * 8);
  uint64_t tm1_tag = 0, tm1_data = 0, tm1_addr = tm1_base + (tm0_addr-DRAM_BASE) / (linesz * 8);
  uint64_t tt_wmask = (((uint64_t)1 << (tagsz*bytes/8)) - 1) << (addr % (64 / tagsz)) * (64 / tagsz);
  uint64_t tt_wdata = read_mem(tt_addr);
  uint64_t tm0_wmask = (uint64_t)1 << (tt_addr % (linesz * 8)) * (linesz * 8);
  uint64_t tm1_wmask = (uint64_t)1 << (tm0_addr % (linesz * 8)) * (linesz * 8);


  enum state_t {s_idle, s_tt_r, s_tm0_r, s_tm1_fr, s_tm0_fr, s_tm0_c, s_tt_fr, s_tt_c, s_tt_w, s_tm0_w, s_tm1_w};

  state_t state = s_tt_r;
  uint64_t rv = 0;

  do {
    switch(state) {
    case s_tt_r:                // read tag table
      tt_tag = read(tt_addr, tt_data, 0);
      if(tt_tag & VALID)        // hit
        state = store && (tt_wdata & tt_wmask) != (tt_data & tt_wmask) ? s_tt_w : s_idle;
      else
        state = s_tm0_r;
      break;
    case s_tm0_r:               // read tag map 0
      tm0_tag = read(tm0_addr, tm0_data, 0);
      if(tm0_tag & VALID) {     // hit
        if(tm0_data & tm0_wmask)   // tagFlag = 1
          state = s_tt_fr;
        else
          state = store && (tt_wdata & tt_wmask) != 0 ? s_tt_c : s_idle;
      } else
        state = s_tm1_fr;
      break;
    case s_tm1_fr:
      tm1_tag = read(tm1_addr, tm1_data, 1);
      if(tm1_data & tm1_wmask)     // tagFlag = 1
        state = s_tm0_fr;
      else
        state = store && (tt_wdata & tt_wmask) != 0 ? s_tm0_c : s_idle;
      break;
    case s_tm0_fr:
      tm0_tag = read(tm0_addr, tm0_data, 1);
      if(tm0_data & tm0_wmask)     // tagFlag = 1
        state = s_tt_fr;
      else
        state = store && (tt_wdata & tt_wmask) != 0 ? s_tt_c : s_idle;
      break;
    case s_tm0_c:
      tm0_tag = create(tm0_addr, 0, 0); // empty tag map 0 block
      state = s_tt_c;
      break;
    case s_tt_fr:
      tt_tag = read(tt_addr, tt_data, 1);
      state = store && (tt_wdata & tt_wmask) != (tt_data & tt_wmask) ? s_tt_w : s_idle;
      break;
    case s_tt_c:
      tt_tag = create(tt_addr, tt_wdata, tt_wmask);
      state = s_tm0_w;
      break;
    case s_tt_w:
      tt_tag = write(tt_addr, tt_wdata, tt_wmask);
      state = (tt_tag & DIRTY) ? s_tm0_w : s_idle;
      break;
    case s_tm0_w:
      tm0_tag = write(tm0_addr, (tt_tag & TAGFLAG) ? tm0_wmask : 0, tm0_wmask);
      state = (tm0_tag & DIRTY) ? s_tm1_w : s_idle;
      break;
    case s_tm1_w:
      tm1_tag = write(tm1_addr, (tm0_tag & TAGFLAG) ? tm1_wmask : 0, tm1_wmask);
      state = s_idle;
      break;
    default:
      assert(state == s_idle);
      // even idle is not reachable here
    }
  } while(state != s_idle);
  return rv;
}
