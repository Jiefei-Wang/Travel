#include "class_Filesystem_file_data.h"
#include "utils.h"
#include "package_settings.h"

Filesystem_file_data::Filesystem_file_data(int coerced_type,
                                           const Subset_index &index,
                                           const Travel_altrep_info &altrep_info) : altrep_info(altrep_info),
                                                                                    coerced_type(coerced_type),
                                                                                    index(index)
{
    if (altrep_info.type == 0)
    {
        Rf_error("Unspecified vector type!\n");
    }
    if (altrep_info.operations.get_region == NULL &&
        altrep_info.operations.read_blocks == NULL)
    {
        Rf_error("The function <get_region> and <read_blocks> are NULL!\n");
    }
    if (altrep_info.operations.serialize != R_NilValue &&
        altrep_info.operations.unserialize == R_NilValue)
    {
        Rf_warning("The serialize function is defined but unserialize is not.\n");
        this->altrep_info.operations.serialize = R_NilValue;
    }
    unit_size = get_type_size(coerced_type);
    file_length = index.length;
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

std::vector<Region_info> Filesystem_file_data::get_cache_region_offset()
{
    std::vector<Region_info> region_offset;
    region_offset.reserve(write_cache.size());
    for (const auto &i : write_cache)
    {
        Region_info info;
        info.start_offset = get_cache_offset(i.first);
        info.size = get_file_read_size(file_size,
                                       info.start_offset,
                                       i.second.get_size());
        info.end_offset = info.start_offset + info.size;
        region_offset.push_back(info);
    }
    return region_offset;
}
Exported_file_data Filesystem_file_data::serialize()
{
    Exported_file_data data;
    data.unit_size = unit_size;
    data.file_length = file_length;
    data.file_size = file_size;
    data.source_length = altrep_info.length;
    data.cache_size = cache_size;
    data.coerced_type = coerced_type;
    data.index = index;
    return data;
}