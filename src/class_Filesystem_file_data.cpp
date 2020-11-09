#include "class_Filesystem_file_data.h"
#include "utils.h"
#include "package_settings.h"

Filesystem_file_data::Filesystem_file_data(int coerced_type,
                                           const Subset_index &index,
                                           const Travel_altrep_info &altrep_info) : altrep_info(altrep_info),
                                                                                    coerced_type(coerced_type),
                                                                                    index(index)
{
  unit_size = get_type_size(coerced_type);
  file_length = index.get_length(altrep_info.length);
  file_size = file_length * unit_size;
  cache_size = CACHE_SIZE;
  claim(cache_size % unit_size == 0);
}


size_t Filesystem_file_data::get_cache_id(size_t data_offset)
{
  return data_offset / cache_size;
}

size_t Filesystem_file_data::get_cache_offset(size_t cache_id)
{
  return cache_size * cache_id;
}
size_t Filesystem_file_data::get_cache_size(size_t cache_id)
{
  size_t true_size = get_file_read_size(file_size, get_cache_offset(cache_id), cache_size);
  return true_size;
}

bool Filesystem_file_data::has_cache_id(size_t cache_id)
{
  return write_cache.find(cache_id) != write_cache.end();
}
Cache_block &Filesystem_file_data::get_cache_block(size_t cache_id)
{
  return write_cache.at(cache_id);
}