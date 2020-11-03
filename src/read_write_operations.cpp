#include <memory>
#include <Rcpp.h>
#include "read_write_operations.h"
//#include "Travel.h"
#include "utils.h"

static size_t read_local_source(Travel_altrep_info* altrep_info, char *buffer, size_t offset, size_t length){
    return altrep_info->operations.get_region(altrep_info, buffer, offset, length);
}
static size_t read_source(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t length){
    return read_local_source(&file_data.altrep_info, buffer, offset, length);
}
/*
read the vector from the file source. 
the unit of offset and length are the element of the vector
The index is based on offset and the space between each element is step
*/
static size_t read_source_with_offset(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t length)
{
    Travel_altrep_info& altrep_info = file_data.altrep_info;
    size_t &read_offset = file_data.start_offset;
    size_t &step = file_data.step;
    offset = offset + read_offset;
    size_t read_size =0;
    if (step == 1)
    {
        read_size = read_source(file_data, buffer, offset, length);
    }else{
        size_t type_size = get_type_size(file_data.altrep_info.type);
        for(size_t i = 0;i<length;i++){
            size_t current_read_size = read_source(file_data, buffer + i*type_size, offset + i*step, 1);
            read_size = read_size + current_read_size;
            if(current_read_size!=1){
                return read_size;
            }
        }
    }
    return read_size;
}

/*
    Read the data with the alignment
*/
static size_t read_file_source_with_alignment(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t size)
{
    Travel_altrep_info& altrep_info = file_data.altrep_info;
    uint8_t unit_size = get_type_size(altrep_info.type);
    size_t misalign_begin = offset % unit_size;
    size_t misalign_end = (offset + size) % unit_size;
    // If the unit size is 1, there is nothing to do
    if (unit_size == 1 ||
        (misalign_begin == 0 && misalign_end == 0))
    {
        //size_t read_length = altrep_info.operations.get_region(&altrep_info, buffer, offset / unit_size, size / unit_size);
        size_t read_length = read_source_with_offset(file_data, buffer, offset / unit_size, size / unit_size);
        return read_length * unit_size;
    }
    //Otherwise, we must convert the request to fit the unit size requirement
    size_t buffer_offset = 0;
    std::unique_ptr<char[]> ptr;
    //Fill the begin of the data if needed
    size_t offset_begin = offset / unit_size;
    if (misalign_begin != 0)
    {
        ptr.reset(new char[unit_size]);
        //size_t read_length = altrep_info.operations.get_region(&altrep_info, ptr.get(), offset_begin, 1);
        size_t read_length = read_source_with_offset(file_data, ptr.get(), offset_begin, 1);
        if (read_length != 1)
        {
            filesystem_log("Warning in read_local_file: The return size of the reading function is not 1!\n");
            return buffer_offset;
        }
        size_t copy_size = std::min(size, unit_size - misalign_begin);
        claim(copy_size + misalign_begin < unit_size);
        claim(copy_size <= size);
        memcpy(buffer, ptr.get() + misalign_begin, copy_size);
        buffer_offset = buffer_offset + copy_size;
        offset_begin++;
    }
    if (buffer_offset >= size)
    {
        return buffer_offset;
    }

    size_t offset_end = (offset + size) / unit_size;
    //Fill the middle of the data
    size_t length = offset_end - offset_begin;
    if (length > 0)
    {
        claim(buffer_offset + length * unit_size <= size);
        claim(buffer_offset + (length + 1) * unit_size > size);
        /* size_t read_length = altrep_info.operations.get_region(&altrep_info,
                                                               buffer + buffer_offset,
                                                               offset_begin, length);*/

        size_t read_length = read_source_with_offset(file_data,
                                                     buffer + buffer_offset,
                                                     offset_begin, length);
        buffer_offset = buffer_offset + read_length * unit_size;
        if (read_length < length)
            return buffer_offset;
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
            return buffer_offset;
        }
        claim(misalign_end < unit_size);
        claim(buffer_offset + misalign_end == size);
        memcpy(buffer + buffer_offset, ptr.get(), misalign_end);
        buffer_offset = buffer_offset + misalign_end;
    }
    if (buffer_offset != size)
    {
        filesystem_log("Warning in read_local_file: Final local read size mismatch, expected: %llu, actual: %llu\n",
                       size, buffer_offset);
    }
    claim(buffer_offset <= size);
    return buffer_offset;
}
static size_t read_file_source_with_alignment(Filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    return read_file_source_with_alignment(file_data, (char *)buffer, offset, size);
}

static size_t buffer_size = 0;
static std::unique_ptr<char[]> read_buffer;
/*
Coerce the data that is read from the file source
*/
static size_t read_source_with_coercion(Filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    if (file_data.coerced_type == file_data.altrep_info.type)
    {
        return read_file_source_with_alignment(file_data, buffer, offset, size);
    }
    size_t source_unit_size = get_type_size(file_data.altrep_info.type);
    size_t file_unit_size = get_type_size(file_data.coerced_type);
    size_t misalign_begin = offset % file_unit_size;
    size_t misalign_end = (offset + size) % file_unit_size;
    //Calculate which region should be read from the source data
    size_t source_length = (size + misalign_begin - misalign_end) / file_unit_size + (misalign_end != 0);
    size_t source_size = source_length * source_unit_size;
    size_t source_offset = offset / file_unit_size * source_unit_size;

    size_t required_buffer_size = std::max(
        size,
        source_size);
    RESERVE_BUFFER(read_buffer, buffer_size, required_buffer_size);
    size_t true_read_size = read_file_source_with_alignment(file_data, read_buffer.get(), source_offset, source_size);
    if (misalign_begin == 0 && misalign_end == 0)
    {
        copy_memory(file_data.coerced_type, file_data.altrep_info.type,
                    buffer, read_buffer.get(), source_length);
    }
    else
    {
        copy_memory(file_data.coerced_type, file_data.altrep_info.type,
                    read_buffer.get(), read_buffer.get(), source_length,
                    file_unit_size > source_unit_size);
        memcpy(buffer, read_buffer.get() + misalign_begin, size);
    }
    RELEASE_BUFFER(read_buffer, buffer_size);
    return true_read_size / source_unit_size * file_unit_size;
}

/*
Read the file data from cache first,
then from the source with coercion

No alignment is required
*/
static size_t read_data_with_cache(Filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    size_t unit_size = get_type_size(file_data.coerced_type);
    claim(offset + size <= file_data.file_size);
    claim(offset % unit_size == 0);
    claim((offset + size) % unit_size == 0);

    size_t &cache_size = file_data.cache_size;
    size_t lowest_block_id = offset / cache_size;
    size_t highest_block_id = (offset + size) / cache_size;
    //Find the iterator that points to the block whose id is not less than lowest_block_id
    auto it = file_data.write_cache.lower_bound(lowest_block_id);
    //The data that has been read
    size_t buffer_offset = 0;
    while (buffer_offset < size)
    {
        //Current block id
        size_t block_id = (offset + buffer_offset) / cache_size;
        //If no cached block exists or the block id exceeds highest_block_id
        if (it == file_data.write_cache.end() || it->first > highest_block_id)
        {
            size_t expect_read_size = size - buffer_offset;
            size_t read_size = read_source_with_coercion(file_data, (char *)buffer + buffer_offset,
                                                                    offset + buffer_offset, expect_read_size);
            if (read_size != expect_read_size)
            {
                filesystem_log("Warning in read_data_with_cache: Read size mismatch, expected: %llu, actual: %llu\n",
                               expect_read_size, read_size);
            }
            buffer_offset = buffer_offset + read_size;
            return buffer_offset;
            break;
        }
        //If the current region is cached
        if (block_id == it->first)
        {
            size_t block_offset = (offset + buffer_offset) % cache_size;
            size_t block_read_length = std::min(cache_size - block_offset, size - buffer_offset);
            const char *block_ptr = it->second.get_const();
            claim(block_ptr != NULL);
            claim(block_offset + block_read_length <= cache_size);
            claim(buffer_offset + block_read_length <= size);
            memcpy((char *)buffer + buffer_offset, block_ptr + block_offset, block_read_length);
            buffer_offset = buffer_offset + block_read_length;
            ++it;
        }
        else if (block_id < it->first)
        {
            //If the current region is not cached but
            //there exists other cached block
            size_t next_block_offset = it->first * cache_size;
            size_t expect_read_size = next_block_offset - (offset + buffer_offset);
            claim(buffer_offset + expect_read_size <= size);
            size_t read_size = read_source_with_coercion(file_data, (char *)buffer + buffer_offset,
                                                                    offset + buffer_offset, expect_read_size);
            if (read_size != expect_read_size)
            {
                filesystem_log("Warning in read_data_with_cache: Read size mismatch, expected: %llu, actual: %llu\n",
                               expect_read_size, read_size);
            }
            buffer_offset = buffer_offset + read_size;
            if (read_size != expect_read_size)
            {
                break;
            }
        }
    }
    if (buffer_offset != size)
    {
        filesystem_log("Warning in read_data_with_cache: Final read size mismatch, expected: %llu, actual: %llu\n",
                       size, buffer_offset);
    }
    claim(buffer_offset <= size);
    return buffer_offset;
}

/*
Read order:
cache layer: read_data_with_cache
coercion layer: read_source_with_coercion
source alignment layer: read_file_source_with_alignment
source offset layer: read_source_with_offset
source layer: read_source
*/
size_t general_read_func(Filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    return read_data_with_cache(file_data, buffer, offset, size);
}

size_t general_write_func(Filesystem_file_data &file_data, const void *buffer, size_t offset, size_t size)
{
    claim(offset + size <= file_data.file_size);
    if (size == 0)
        return 0;
    size_t &cache_size = file_data.cache_size;
    size_t buffer_offset = 0;
    while (buffer_offset < size)
    {
        size_t block_id = (offset + buffer_offset) / cache_size;
        size_t block_offset = (offset + buffer_offset) % cache_size;
        size_t block_write_length = std::min(cache_size - block_offset, size - buffer_offset);

        if (file_data.write_cache.find(block_id) == file_data.write_cache.end())
        {
            filesystem_log("Creating new block %llu\n", block_id);
            //file_data.write_cache[block_id] = Cache_block(cache_size);
            Cache_block block(cache_size);
            if (block_offset != 0 || block_write_length != cache_size)
            {
                size_t block_offset_in_file = block_id * cache_size;
                size_t expect_read_size = get_file_read_size(file_data.file_size, block_offset_in_file, cache_size);
                size_t read_size = general_read_func(file_data, block.get(),
                                                     block_offset_in_file, expect_read_size);
                if (read_size != expect_read_size)
                {
                    filesystem_log("Warning in general_write_func: Read size mismatch, expected: %llu, actual: %llu\n",
                                   expect_read_size, read_size);
                }
            }
            file_data.write_cache.insert(std::pair<size_t, Cache_block>(block_id, block));
        }
        char *block_ptr = file_data.write_cache.find(block_id)->second.get();
        claim(buffer_offset + block_write_length <= size);
        claim(block_offset + block_write_length <= cache_size);
        memcpy(block_ptr + block_offset, (char *)buffer + buffer_offset, block_write_length);
        buffer_offset = buffer_offset + block_write_length;
    }
    claim(buffer_offset <= size);
    return buffer_offset;
}