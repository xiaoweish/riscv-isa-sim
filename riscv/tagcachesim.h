// See LICENSE for license details.

#ifndef _RISCV_TAG_CACHE_SIM_H
#define _RISCV_TAG_CACHE_SIM_H

#include "memtracer.h"
#include "cachesim.h"
#include "sim.h"

class tag_cache_sim_t : public cache_sim_t
{
 public:
  tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, sim_t* sim);
  tag_cache_sim_t(const tag_cache_sim_t& rhs);
  virtual ~tag_cache_sim_t();

  virtual uint64_t access(uint64_t addr, size_t bytes, bool store) = 0;

 protected:
  size_t tagsz;
  uint64_t tag_base;

  uint8_t *datas;
  tag_cache_sim_t* tag_map;

  sim_t *sim;

  virtual void init();

  static const uint64_t TAGFLAG = 1ULL << 61;
  uint64_t* check_tag(uint64_t addr, size_t &row);
  uint64_t* victimize(uint64_t addr, size_t &row);
  void refill(uint64_t addr, size_t row);
  void writeback(size_t row);
  size_t data_row_addr(uint64_t addr, size_t row) {
    return row*linesz + (addr%linesz & ~(uint64_t)(0x07));
  }
  size_t memsz() { return sim->memsz; }
  uint64_t read_mem(uint64_t addr) {
    return *(uint64_t *)(sim->addr_to_mem(addr & ~(uint64_t)(0x7)));
  }
  void verify(size_t row);  // verify that the row data match with actual memory data

  uint64_t read(uint64_t addr, uint64_t &data, uint8_t fetch);
  uint64_t write(uint64_t addr, uint64_t data, uint64_t mask);
  uint64_t create(uint64_t addr, uint64_t data, uint64_t mask);
};

class tag_table_sim_t : public tag_cache_sim_t
{
 public:
  tag_table_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, sim_t* sim);
  tag_table_sim_t(const tag_table_sim_t &rhs);
  static tag_cache_sim_t* construct(const char* config, const char* name);
  virtual uint64_t access(uint64_t addr, size_t bytes, bool store);

 private:
  uint64_t tt_base;
  virtual void init();
};

class tag_map_sim_t : public tag_cache_sim_t
{
 public:
  tag_map_sim_t(size_t sets, size_t ways, size_t linesz, uint64_t tablesz, uint64_t table_linesz, const char* name, sim_t* sim);
  tag_map_sim_t(const tag_map_sim_t &rhs);
  static tag_cache_sim_t* construct(const char* config, const char* name);
  virtual uint64_t access(uint64_t addr, size_t bytes, bool store);

 private:
  uint64_t tt_size;
  uint64_t tt_linesz;
  uint64_t tm_base;
  virtual void init();
};

class unified_tag_cache_sim_t : public tag_cache_sim_t
{
 public:
  unified_tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, sim_t* sim);
  unified_tag_cache_sim_t(const unified_tag_cache_sim_t& rhs);
  static tag_cache_sim_t* construct(const char* config, const char* name);
  virtual uint64_t access(uint64_t addr, size_t bytes, bool store);

 private:
  uint64_t tt_base, tm0_base, tm1_base;

  virtual void init();

};




#endif
