// See LICENSE for license details.

#ifndef _RISCV_TAG_CACHE_SIM_H
#define _RISCV_TAG_CACHE_SIM_H

#include "memtracer.h"
#include "cachesim.h"
#include "sim.h"
#include <vector>

class tag_cache_sim_t : public cache_sim_t
{
 public:
  tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, const sim_t* sim);
  tag_cache_sim_t(const tag_cache_sim_t& rhs);
  virtual ~tag_cache_sim_t();

 protected:
  size_t tagsz;

  uint8_t *datas;
  tag_cache_sim_t* tag_map;

  const sim_t *sim;

  virtual void init();
};

class tag_table_sim_t : public tag_cache_sim_t
{
 public:
  tag_table_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, const sim_t* sim);
  static tag_cache_sim_t* construct(const char* config, const char* name);
};

class tag_map_sim_t : public tag_cache_sim_t
{
 public:
  tag_map_sim_t(size_t sets, size_t ways, size_t linesz, const char* name, const sim_t* sim);
  static tag_cache_sim_t* construct(const char* config, const char* name);
};

class unified_tag_cache_sim_t : public tag_cache_sim_t
{
 public:
  unified_tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, const sim_t* sim);
  static tag_cache_sim_t* construct(const char* config, const char* name);
};




#endif
