#include "filesystem_manager.h"
#include "Filesystem_cache_copier.h"
#include "read_write_operations.h"
#include "utils.h"

Filesystem_cache_copier::Filesystem_cache_copier(
    Filesystem_file_data &dest_file_info,
    Filesystem_file_data &source_file_info) : dest_file_info(dest_file_info),
                                              source_file_info(source_file_info)
{
}
Filesystem_cache_copier::~Filesystem_cache_copier()
{
    flush_buffer();
}
void Filesystem_cache_copier::copy(size_t dest_index, size_t source_index)
{
    //The offset of the data in the source file measured in byte
    size_t source_data_offset = source_file_info.get_data_offset(source_index);
    load_source_cache(source_data_offset);
    size_t dest_data_offset = dest_file_info.get_data_offset(dest_index);
    load_dest_cache(dest_data_offset);
    //The offset of the beginning of the cache block in the source file measured in byte
    size_t source_cache_offset = source_file_info.get_cache_offset_by_data_offset(source_data_offset);
    size_t dest_cache_offset = dest_file_info.get_cache_offset_by_data_offset(dest_data_offset);
    //The offset of the data within the cache measured in bytes
    size_t source_within_cache_offset = source_data_offset - source_cache_offset;
    size_t dest_within_cache_offset = dest_data_offset - dest_cache_offset;
    copy_memory(dest_file_info.coerced_type, source_file_info.coerced_type,
                dest_cache_ptr.get() + dest_within_cache_offset,
                source_cache_ptr + source_within_cache_offset, 1, false);
}
void Filesystem_cache_copier::load_source_cache(size_t source_data_offset)
{
    size_t current_source_cache_id = source_file_info.get_cache_id(source_data_offset);
    if (source_cache_id != current_source_cache_id ||
        source_cache_ptr == nullptr)
    {
        claim(source_file_info.has_cache_id(current_source_cache_id));
        source_cache_id = current_source_cache_id;
        source_cache_ptr = source_file_info.get_cache_block(source_cache_id).get_const();
    }
}
void Filesystem_cache_copier::load_dest_cache(size_t dest_data_offset)
{
    size_t current_dest_cache_id = dest_file_info.get_cache_id(dest_data_offset);
    if (dest_cache_id == current_dest_cache_id)
    {
        //If the required cache id is not loaded and the cache is empty
        //load the required cache
        if (dest_cache_ptr.get() == nullptr)
        {
            dest_cache_ptr.reset(new char[dest_file_info.cache_size]);
            general_read_func(dest_file_info, dest_cache_ptr.get(),
                              dest_cache_id * dest_file_info.cache_size,
                              dest_file_info.cache_size);
        }
    }
    else
    {
        //If the required cache id is not loaded but there exist old cache
        if (dest_cache_ptr.get() != nullptr)
        {
            flush_buffer();
        }
        //load the required cache
        dest_cache_id = current_dest_cache_id;
        general_read_func(dest_file_info, dest_cache_ptr.get(),
                          dest_cache_id * dest_file_info.cache_size,
                          dest_file_info.cache_size);
    }
}
void Filesystem_cache_copier::flush_buffer()
{
    general_write_func(dest_file_info, dest_cache_ptr.get(),
                       dest_cache_id * dest_file_info.cache_size,
                       dest_file_info.cache_size);
}
