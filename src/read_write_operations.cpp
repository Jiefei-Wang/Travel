#include <memory>
#include <Rcpp.h>
#include "read_write_operations.h"
//#include "Travel.h"
#include "utils.h"

static size_t read_local_source(Travel_altrep_info *altrep_info, char *buffer, size_t offset, size_t length)
{
    return altrep_info->operations.get_region(altrep_info, buffer, offset, length);
}
static size_t read_source(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t length)
{
    return read_local_source(&file_data.altrep_info, buffer, offset, length);
}
/*
read the vector from the file source. 
the unit of offset and length are the element of the vector
The index is based on offset and the space between each element is step
*/
static size_t read_source_with_offset(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t length)
{
    Travel_altrep_info &altrep_info = file_data.altrep_info;
    size_t &read_offset = file_data.start_offset;
    size_t &step = file_data.step;
    offset = offset + read_offset;
    size_t read_size = 0;
    if (step == 1)
    {
        read_size = read_source(file_data, buffer, offset, length);
    }
    else
    {
        size_t type_size = get_type_size(file_data.altrep_info.type);
        for (size_t i = 0; i < length; i++)
        {
            size_t current_read_size = read_source(file_data, buffer + i * type_size, offset + i * step, 1);
            read_size = read_size + current_read_size;
            if (current_read_size != 1)
            {
                return read_size;
            }
        }
    }
    return read_size;
}


static size_t buffer_size = 0;
static std::unique_ptr<char[]> read_buffer;
/*
Coerce the data that is read from the file source

the return value is the number of the element that has been read
*/
static size_t read_source_with_coercion(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t length)
{
    if (file_data.coerced_type == file_data.altrep_info.type)
    {
        return read_source_with_offset(file_data, buffer, offset, length);
    }
    uint8_t source_unit_size = get_type_size(file_data.altrep_info.type);
    uint8_t& data_unit_size = file_data.unit_size;
    

    size_t required_buffer_size = std::max(
        length*data_unit_size,
        length*source_unit_size);
    
    RESERVE_BUFFER(read_buffer, buffer_size, required_buffer_size);
    size_t true_read_size = read_source_with_offset(file_data, read_buffer.get(), source_offset, source_size);
        copy_memory(file_data.coerced_type, file_data.altrep_info.type,
                    buffer, read_buffer.get(), source_length);
    RELEASE_BUFFER(read_buffer, buffer_size);
    return true_read_size / source_unit_size * data_unit_size;
}



/*
    Read the data with the alignment
    the downstream function would not need to align the offset with
    the element size
*/
static size_t read_file_source_with_alignment(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t size)
{
    uint8_t& unit_size = file_data.unit_size;
    size_t misalign_begin = offset % unit_size;
    size_t misalign_end = (offset + size) % unit_size;
    // If the unit size is 1, there is nothing to do
    if (unit_size == 1 ||
        (misalign_begin == 0 && misalign_end == 0))
    {
        //size_t read_length = altrep_info.operations.get_region(&altrep_info, buffer, offset / unit_size, size / unit_size);
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
        //size_t read_length = altrep_info.operations.get_region(&altrep_info, ptr.get(), offset_begin, 1);
        size_t read_length = read_source_with_coercion(file_data, ptr.get(), offset_begin, 1);
        if (read_length != 1)
        {
            filesystem_log("Warning in read_local_file: The return size of the reading function is not 1!\n");
            return buffer_read_size;
        }
        size_t copy_size = std::min(size, unit_size - misalign_begin);
        claim(copy_size + misalign_begin <= unit_size);
        claim(copy_size <= size);
        memcpy(buffer, ptr.get() + misalign_begin, copy_size);
        buffer_read_size = buffer_read_size + copy_size;
        offset_begin++;
    }
    if (buffer_read_size >= size)
    {
        return buffer_read_size;
    }

    size_t offset_end = (offset + size) / unit_size;
    //Fill the middle of the data
    size_t length = offset_end - offset_begin;
    if (length > 0)
    {
        claim(buffer_read_size + length * unit_size <= size);
        claim(buffer_read_size + (length + 1) * unit_size > size);
        /* size_t read_length = altrep_info.operations.get_region(&altrep_info,
                                                               buffer + buffer_offset,
                                                               offset_begin, length);*/

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
        //size_t read_length = altrep_info.operations.get_region(&altrep_info,
        //                                                       ptr.get(), offset_end, 1);
        size_t read_length = read_source_with_offset(file_data, ptr.get(), offset_end, 1);
        if (read_length != 1)
        {
            filesystem_log("Warning in read_local_file: The return size of the reading function is not 1!\n");
            return buffer_read_size;
        }
        claim(misalign_end < unit_size);
        claim(buffer_read_size + misalign_end == size);
        memcpy(buffer + buffer_read_size, ptr.get(), misalign_end);
        buffer_read_size = buffer_read_size + misalign_end;
    }
    if (buffer_read_size != size)
    {
        filesystem_log("Warning in read_local_file: Final local read size mismatch, expected: %llu, actual: %llu\n",
                       size, buffer_read_size);
    }
    claim(buffer_read_size <= size);
    return buffer_read_size;
}


/*
Read the file data from cache first,
then from the downstream function

The offset has no alignment requirement
*/
static size_t read_data_with_cache(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t size)
{
    uint8_t &unit_size = file_data.unit_size;
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
            size_t read_size = read_source_with_coercion(file_data, buffer + buffer_read_size,
                                                         current_data_offset, expect_read_size);
            buffer_read_size = buffer_read_size + read_size;
            if (read_size != expect_read_size)
            {
                filesystem_log("Warning in read_data_with_cache: Read size mismatch, expected: %llu, actual: %llu\n",
                               expect_read_size, read_size);
                break;
            }
        }
        //If the current region is cached
        if (cache_id == it->first)
        {
            size_t within_cache_offset = (current_data_offset) % cache_size;
            size_t expect_read_size = std::min(cache_size - within_cache_offset, size - buffer_read_size);
            const char *cache_ptr = it->second.get_const();
            claim(cache_ptr != NULL);
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
            size_t expect_read_size = next_cache_offset - (offset + buffer_read_size);
            claim(buffer_read_size + expect_read_size <= size);
            size_t read_size = read_source_with_coercion(file_data, buffer + buffer_read_size,
                                                         offset + buffer_read_size, expect_read_size);
            buffer_read_size = buffer_read_size + read_size;
            if (read_size != expect_read_size)
            {
                filesystem_log("Warning in read_data_with_cache: Read size mismatch, expected: %llu, actual: %llu\n",
                               expect_read_size, read_size);
                break;
            }
        }
    }
    if (buffer_read_size != size)
    {
        filesystem_log("Warning in read_data_with_cache: Final read size mismatch, expected: %llu, actual: %llu\n",
                       size, buffer_read_size);
    }
    claim(buffer_read_size <= size);
    return buffer_read_size;
}

/*
Read order:
cache layer: read_data_with_cache
source alignment layer: read_file_source_with_alignment
coercion layer: read_source_with_coercion
source offset layer: read_source_with_offset
source layer: read_source
*/
size_t general_read_func(Filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    size = std::min(zero_bounded_minus(file_data.file_size, offset), size);
    return read_data_with_cache(file_data, (char*)buffer, offset, size);
}

size_t general_write_func(Filesystem_file_data &file_data, const void *buffer, size_t offset, size_t size)
{
    size = std::min(zero_bounded_minus(file_data.file_size, offset), size);
    claim(offset + size <= file_data.file_size);
    if (size == 0)
        return 0;
    size_t &cache_size = file_data.cache_size;
    size_t buffer_offset = 0;
    while (buffer_offset < size)
    {
        size_t cache_id = (offset + buffer_offset) / cache_size;
        size_t cache_offset = (offset + buffer_offset) % cache_size;
        size_t cache_write_length = std::min(cache_size - cache_offset, size - buffer_offset);

        if (file_data.write_cache.find(cache_id) == file_data.write_cache.end())
        {
            filesystem_log("Creating new cache %llu\n", cache_id);
            //file_data.write_cache[cache_id] = Cache_block(cache_size);
            Cache_block cache(cache_size);
            if (cache_offset != 0 || cache_write_length != cache_size)
            {
                size_t cache_offset_in_file = cache_id * cache_size;
                size_t expect_read_size = get_file_read_size(file_data.file_size, cache_offset_in_file, cache_size);
                size_t read_size = general_read_func(file_data, cache.get(),
                                                     cache_offset_in_file, expect_read_size);
                if (read_size != expect_read_size)
                {
                    filesystem_log("Warning in general_write_func: Read size mismatch, expected: %llu, actual: %llu\n",
                                   expect_read_size, read_size);
                }
            }
            file_data.write_cache.insert(std::pair<size_t, Cache_block>(cache_id, cache));
        }
        char *cache_ptr = file_data.write_cache.find(cache_id)->second.get();
        claim(buffer_offset + cache_write_length <= size);
        claim(cache_offset + cache_write_length <= cache_size);
        memcpy(cache_ptr + cache_offset, (char *)buffer + buffer_offset, cache_write_length);
        buffer_offset = buffer_offset + cache_write_length;
    }
    claim(buffer_offset <= size);
    return buffer_offset;
}