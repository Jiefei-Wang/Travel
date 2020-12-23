#include "class_Filesystem_cache_copier.h"
#include "read_write_operations.h"
#include "utils.h"

Filesystem_cache_copier::Filesystem_cache_copier(
    Filesystem_file_data &dest_file_data,
    Filesystem_file_data &source_file_data) : dest_file_data(dest_file_data),
                                              source_file_data(source_file_data)
{
}
Filesystem_cache_copier::~Filesystem_cache_copier()
{
    //flush_buffer();
}
void Filesystem_cache_copier::copy(size_t dest_index, size_t source_index)
{
    //The offset of the data in the source file measured in byte
    size_t source_data_offset = source_index * source_file_data.unit_size;
    size_t dest_data_offset = dest_index * dest_file_data.unit_size;
    //Load the cache
    load_source_cache(source_data_offset);
    load_dest_cache(dest_data_offset);
    //The offset of the beginning of the cache block in the file measured in byte
    size_t source_cache_offset = source_file_data.get_cache_offset(source_cache_id);
    size_t dest_cache_offset = dest_file_data.get_cache_offset(dest_cache_id);
    //The offset of the data within the cache measured in bytes
    size_t source_within_cache_offset = source_data_offset - source_cache_offset;
    size_t dest_within_cache_offset = dest_data_offset - dest_cache_offset;
    assert(source_within_cache_offset + source_file_data.unit_size<=source_file_data.cache_size);
    assert(dest_within_cache_offset + dest_file_data.unit_size<=dest_file_data.cache_size);
    covert_data(dest_file_data.coerced_type, source_file_data.coerced_type,
                dest_cache_ptr + dest_within_cache_offset,
                source_cache_ptr + source_within_cache_offset, 1, false);
}
void Filesystem_cache_copier::load_source_cache(size_t source_data_offset)
{
    size_t current_source_cache_id = source_file_data.get_cache_id(source_data_offset);
    if (source_cache_id != current_source_cache_id ||
        source_cache_ptr == nullptr)
    {
        assert(source_file_data.has_cache_id(current_source_cache_id));
        source_cache_id = current_source_cache_id;
        source_cache_ptr = source_file_data.get_cache_block(source_cache_id).get_const();
    }
}
void Filesystem_cache_copier::load_dest_cache(size_t dest_data_offset)
{
    size_t current_dest_cache_id = dest_file_data.get_cache_id(dest_data_offset);
    if (dest_cache_id != current_dest_cache_id ||
        dest_cache_ptr == nullptr)
    {
        dest_cache_id = current_dest_cache_id;
        dest_cache_ptr = (char*)load_cache(dest_file_data, dest_cache_id);
    }
}
void Filesystem_cache_copier::flush_buffer()
{
    /*general_write_func(dest_file_data, dest_cache_ptr.get(),
                       dest_cache_id * dest_file_data.cache_size,
                       dest_file_data.cache_size);*/
}
