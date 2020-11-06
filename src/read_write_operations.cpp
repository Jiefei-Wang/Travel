#include <memory>
#include <Rcpp.h>
#include "read_write_operations.h"
//#include "Travel.h"
#include "utils.h"

static size_t read_local_source(Travel_altrep_info *altrep_info, char *buffer, size_t offset, size_t length)
{
    size_t true_read_length = altrep_info->operations.get_region(altrep_info, buffer, offset, length);
    if (length != true_read_length)
    {
        filesystem_log("Warning in read_local_source: read length mismatch, "
                       "offset: %llu, expected: %llu, actual: %llu\n",
                       (uint64_t)offset, (uint64_t)length, (uint64_t)true_read_length);
    }
    return true_read_length;
}
static size_t read_source(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t length)
{
    return read_local_source(&file_data.altrep_info, buffer, offset, length);
}
/*
map from the index of the vector to the index of the source vector. 
the unit of offset and length are the element of the vector
*/
static size_t read_source_with_subset(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t length)
{
    Subset_index &subset_index = file_data.subset_index;
    size_t buffer_read_length = 0;
    //If the subset_index is consecutive, we simply
    //find out the start index of the offset in the source file
    if (subset_index.is_consecutive())
    {
        buffer_read_length = read_source(file_data, buffer, subset_index.get_source_index(offset), length);
    }
    else
    {
        //If subset_index is not consecutive, we read it block by block
        size_t &block_length = file_data.subset_index.block_length;
        size_t type_size = get_type_size(file_data.altrep_info.type);
        do
        {
            size_t current_read_offset = offset + buffer_read_length;
            size_t within_block_offset = current_read_offset % block_length;
            size_t expect_read_length = std::min(block_length - within_block_offset, length - buffer_read_length);
            size_t true_read_length = read_source(file_data,
                                                  buffer + buffer_read_length * type_size,
                                                  subset_index.get_source_index(current_read_offset),
                                                  expect_read_length);
            if (true_read_length != expect_read_length)
            {
                break;
            }
            buffer_read_length = buffer_read_length + true_read_length;
        } while (buffer_read_length < length);
    }
    return buffer_read_length;
}

//This buffer will hold a certain amount of data in memory
static Unique_buffer coercion_buffer;
/*
Coerce the data that is read from the file source
the return value is the number of the element that has been read
*/
static size_t read_source_with_coercion(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t length)
{
    //If the type of the file is the same as the type of the source
    //We just pass the request to the downstream function
    if (file_data.coerced_type == file_data.altrep_info.type)
    {
        return read_source_with_subset(file_data, buffer, offset, length);
    }
    uint8_t source_unit_size = get_type_size(file_data.altrep_info.type);
    uint8_t &file_unit_size = file_data.unit_size;

    size_t required_buffer_size = length * std::max(file_unit_size, source_unit_size);
    coercion_buffer.reserve(required_buffer_size);
    size_t true_read_length = read_source_with_subset(file_data, coercion_buffer.get(), offset, length);
    copy_memory(file_data.coerced_type, file_data.altrep_info.type,
                buffer, coercion_buffer.get(), true_read_length);
    coercion_buffer.release();
    return true_read_length;
}

/*
    Read the data with the alignment
    the downstream function would not need to align the offset with
    the element size
*/
static size_t read_with_alignment(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t size)
{
    uint8_t &unit_size = file_data.unit_size;
    size_t misalign_begin = offset % unit_size;
    size_t misalign_end = (offset + size) % unit_size;
    // If the unit size is 1 or there is no misalignment,
    // we just pass the request to the downstream function
    if (unit_size == 1 ||
        (misalign_begin == 0 && misalign_end == 0))
    {
        size_t read_length = read_source_with_coercion(file_data, buffer, offset / unit_size, size / unit_size);
        return read_length * unit_size;
    }
    //Otherwise, we must convert the request to fit the unit size requirement
    size_t buffer_read_size = 0;
    std::unique_ptr<char[]> ptr;
    //Fill the begin of the data if needed
    size_t offset_begin = offset / unit_size;
    if (misalign_begin != 0)
    {
        ptr.reset(new char[unit_size]);
        size_t read_length = read_source_with_coercion(file_data, ptr.get(), offset_begin, 1);
        if (read_length != 1)
        {
            return buffer_read_size;
        }
        size_t copy_size = std::min(size, unit_size - misalign_begin);
        memcpy(buffer, ptr.get() + misalign_begin, copy_size);
        buffer_read_size = buffer_read_size + copy_size;
        offset_begin++;
    }
    if (buffer_read_size == size)
    {
        return buffer_read_size;
    }
    //If we still need to read the data after reading the first element
    size_t offset_end = (offset + size) / unit_size;
    //Fill the middle of the data
    size_t length = offset_end - offset_begin;
    if (length > 0)
    {
        claim(buffer_read_size + length * unit_size <= size);
        claim(buffer_read_size + (length + 1) * unit_size > size);

        size_t read_length = read_source_with_coercion(file_data,
                                                       buffer + buffer_read_size,
                                                       offset_begin, length);
        buffer_read_size = buffer_read_size + read_length * unit_size;
        if (read_length < length)
            return buffer_read_size;
    }
    //Fill the end of the data
    if (misalign_end != 0)
    {
        if (ptr == nullptr)
            ptr.reset(new char[unit_size]);
        size_t read_length = read_source_with_subset(file_data, ptr.get(), offset_end, 1);
        if (read_length != 1)
        {
            return buffer_read_size;
        }
        memcpy(buffer + buffer_read_size, ptr.get(), misalign_end);
        buffer_read_size = buffer_read_size + misalign_end;
    }
    claim(buffer_read_size == size);
    return buffer_read_size;
}

/*
Read the file data from cache first,
then from the downstream function

The offset has no alignment requirement
*/
static size_t read_data_with_cache(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t size)
{
    size_t &cache_size = file_data.cache_size;
    size_t lowest_cache_id = offset / cache_size;
    size_t highest_cache_id = (offset + size) / cache_size;
    //Find the iterator that points to the cache whose id is not less than lowest_cache_id
    auto it = file_data.write_cache.lower_bound(lowest_cache_id);
    //The data that has been read
    size_t buffer_read_size = 0;
    while (buffer_read_size < size)
    {
        size_t current_data_offset = offset + buffer_read_size;
        //Current cache id
        size_t cache_id = file_data.get_cache_id(current_data_offset);
        //If no cached cache exists or the cache id exceeds highest_cache_id
        if (it == file_data.write_cache.end() || it->first > highest_cache_id)
        {
            size_t expect_read_size = size - buffer_read_size;
            size_t read_size = read_with_alignment(file_data, buffer + buffer_read_size,
                                                   current_data_offset, expect_read_size);
            buffer_read_size = buffer_read_size + read_size;
            if (read_size != expect_read_size)
            {
                return buffer_read_size;
            }else{
                break;
            }
        }
        //If the current region is cached
        if (cache_id == it->first)
        {
            size_t within_cache_offset = (current_data_offset) % cache_size;
            size_t expect_read_size = std::min(cache_size - within_cache_offset, size - buffer_read_size);
            const char *cache_ptr = it->second.get_const();
            claim(within_cache_offset + expect_read_size <= cache_size);
            claim(buffer_read_size + expect_read_size <= size);
            memcpy(buffer + buffer_read_size, cache_ptr + within_cache_offset, expect_read_size);
            buffer_read_size = buffer_read_size + expect_read_size;
            ++it;
        }
        else if (cache_id < it->first)
        {
            //If the current region is not cached but
            //there exists other cached cache
            size_t next_cache_offset = file_data.get_cache_offset(it->first);
            size_t expect_read_size = next_cache_offset - current_data_offset;
            claim(buffer_read_size + expect_read_size <= size);
            size_t read_size = read_with_alignment(file_data, buffer + buffer_read_size,
                                                   offset + buffer_read_size, expect_read_size);
            buffer_read_size = buffer_read_size + read_size;
            if (read_size != expect_read_size)
            {
                return buffer_read_size;
            }
        }
    }
    claim(buffer_read_size == size);
    return buffer_read_size;
}

/*
Read order:
cache layer: read_data_with_cache
source alignment layer: read_with_alignment
coercion layer: read_source_with_coercion
source offset layer: read_source_with_subset
source layer: read_source
*/
size_t general_read_func(Filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    size = std::min(zero_bounded_minus(file_data.file_size, offset), size);
    return read_data_with_cache(file_data, (char *)buffer, offset, size);
}

size_t general_write_func(Filesystem_file_data &file_data, const void *buffer, size_t offset, size_t size)
{
    size = std::min(zero_bounded_minus(file_data.file_size, offset), size);
    claim(offset + size <= file_data.file_size);
    if (size == 0)
        return 0;
    size_t &cache_size = file_data.cache_size;
    size_t write_offset = 0;
    while (write_offset < size)
    {
        size_t unwrite_size = size - write_offset;
        size_t cache_id = file_data.get_cache_id(offset + write_offset);
        size_t within_cache_offset = (offset + write_offset) % cache_size;
        size_t cache_write_size = std::min(cache_size - within_cache_offset, unwrite_size);

        if (!file_data.has_cache_id(cache_id))
        {
            filesystem_log("Creating new cache %llu\n", cache_id);
            //if the data in the cache will be overwritten by the write data,
            //we will just create the empty cache
            if (within_cache_offset == 0 && cache_write_size == cache_size)
            {
                Cache_block cache(cache_size);
                file_data.write_cache.insert(std::pair<size_t, Cache_block>(cache_id, cache));
            }
            else
            {
                load_cache(file_data, cache_id);
            }
        }
        char *cache_ptr = file_data.get_cache_block(cache_id).get();
        claim(write_offset + cache_write_size <= size);
        memcpy(cache_ptr + within_cache_offset, (char *)buffer + write_offset, cache_write_size);
        write_offset = write_offset + cache_write_size;
    }
    claim(write_offset <= size);
    return write_offset;
}

void *load_cache(Filesystem_file_data &file_data, size_t cache_id)
{
    if (file_data.has_cache_id(cache_id))
    {
        return file_data.get_cache_block(cache_id).get();
    }
    size_t cache_offset_in_file = file_data.get_cache_offset(cache_id);
    size_t expect_read_size = file_data.get_cache_size(cache_id);
    Cache_block cache(file_data.cache_size);
    void *ptr = cache.get();
    general_read_func(file_data, ptr,
                      cache_offset_in_file, expect_read_size);
    file_data.write_cache.insert(std::pair<size_t, Cache_block>(cache_id, cache));
    return ptr;
}