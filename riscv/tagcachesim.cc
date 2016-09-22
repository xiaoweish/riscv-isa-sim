// See LICENSE for license details.

#include "tagcachesim.h"

// ----------------- tag cache base class ----------------------- //

tag_cache_sim_t::tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, const sim_t* sim)
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
}

// ----------------- separate tag table class ------------------- //

tag_table_sim_t::tag_table_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, const sim_t* sim)
  : tag_cache_sim_t(sets, ways, linesz, tagsz, name, sim)
{
}

// ----------------- separate tag map class --------------------- //
tag_map_sim_t::tag_map_sim_t(size_t sets, size_t ways, size_t linesz, const char* name, const sim_t* sim)
  : tag_cache_sim_t(sets, ways, linesz, 0, name, sim)
{
}

// ----------------- unified tag cache class -------------------- //
unified_tag_cache_sim_t::unified_tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, size_t tagsz, const char* name, const sim_t* sim)
  : tag_cache_sim_t(sets, ways, linesz, tagsz, name, sim)
{
}

