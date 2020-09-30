#include <memory>
#include <Rcpp.h>
#include "read_write_operations.h"
//#include "Travel.h"
#include "utils.h"

/*
An utility to get the true read size that will not read out-of-bound
*/
size_t get_valid_file_size(size_t file_size, size_t offset, size_t size)
{
    if (offset + size > file_size)
    {
        if (offset >= file_size)
        {
            return 0;
        }
        else
        {
            return file_size - offset;
        }
    }
    else
    {
        return size;
    }
}

/*
    Read the local file
*/
size_t read_local_file_func(filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    unsigned int &unit_size = file_data.unit_size;
    // If the unit size is 1, there is nothing to do
    if (unit_size == 1)
    {
        return file_data.data_func(file_data, buffer, offset, size);
    }
    //Otherwise, we must convert the request to fit the unit size requirement
    size_t buffer_offset = 0;
    char *cbuffer = (char *)buffer;
    std::unique_ptr<char[]> ptr;
    //Fill the begin of the data if needed
    size_t start_offset = offset / unit_size;
    size_t start_intra_elt_offset = offset % unit_size;
    if (start_intra_elt_offset != 0)
    {
        ptr.reset(new char[unit_size]);
        size_t read_length = file_data.data_func(file_data, ptr.get(), start_offset, 1);
        if (read_length != 1)
        {
            filesystem_log("Warning in read_local_file_func: The return size of the reading function is not 1!\n");
            return buffer_offset;
        }
        size_t copy_size = std::min(size, unit_size - start_intra_elt_offset);
        claim(copy_size + start_intra_elt_offset < unit_size);
        claim(copy_size <= size);
        memcpy(cbuffer, ptr.get() + start_intra_elt_offset, copy_size);
        cbuffer = cbuffer;
        buffer_offset = buffer_offset + copy_size;
        start_offset++;
    }
    if (buffer_offset >= size)
    {
        return buffer_offset;
    }

    size_t end_offset = (offset + size) / unit_size;
    size_t end_intra_elt_len = (offset + size) % unit_size;
    //Fill the middle of the data
    size_t length = end_offset - start_offset;
    if (length > 0)
    {
        claim(buffer_offset + length * unit_size <= size);
        claim(size - buffer_offset - length * unit_size < unit_size);
        size_t read_length = file_data.data_func(file_data, cbuffer + buffer_offset,
                                                 start_offset, length);
        buffer_offset = buffer_offset + read_length * unit_size;
        if (read_length < length)
            return buffer_offset;
    }
    //Fill the end of the data
    if (end_intra_elt_len != 0)
    {
        if (ptr == nullptr)
            ptr.reset(new char[unit_size]);
        size_t read_length = file_data.data_func(file_data, ptr.get(), end_offset, 1);
        if (read_length != 1)
        {
            filesystem_log("Warning in read_local_file_func: The return size of the reading function is not 1!\n");
            return buffer_offset;
        }
        claim(end_intra_elt_len < unit_size);
        claim(buffer_offset + end_intra_elt_len == size);
        memcpy(cbuffer + buffer_offset, ptr.get(), end_intra_elt_len);
        buffer_offset = buffer_offset + end_intra_elt_len;
    }
    if (buffer_offset != size)
    {
        filesystem_log("Warning in read_local_file_func: Final local read size mismatch, expected: %llu, actual: %llu\n",
                       size, buffer_offset);
    }
    claim(buffer_offset <= size);
    return buffer_offset;
}
/*
    Read the data from source
*/
size_t read_file_source_func(filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    return read_local_file_func(file_data, buffer, offset, size);
}

size_t general_write_func(filesystem_file_data &file_data, const void *buffer, size_t offset, size_t size)
{
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
            file_data.write_cache[block_id] = new char[cache_size];
            if (block_offset != 0 || block_write_length != cache_size)
            {
                size_t block_offset_in_file = block_id * cache_size;
                size_t expect_read_size = get_valid_file_size(file_data.file_size, block_offset_in_file, cache_size);
                size_t read_size = read_file_source_func(file_data, file_data.write_cache[block_id],
                                                         block_offset_in_file, expect_read_size);
                if (read_size != expect_read_size)
                {
                    filesystem_log("Warning in general_write_func: Read size mismatch, expected: %llu, actual: %llu\n",
                                   expect_read_size, read_size);
                }
            }
        }
        char *block_ptr = file_data.write_cache[block_id];
        claim(buffer_offset + block_write_length <= size);
        claim(block_offset + block_write_length <= cache_size);
        memcpy(block_ptr + block_offset, (char *)buffer + buffer_offset, block_write_length);
        buffer_offset = buffer_offset + block_write_length;
    }
    return buffer_offset;
}

size_t general_read_func(filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    if (size == 0)
        return 0;
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
            claim(buffer_offset <= size);
            size_t expect_read_size = size - buffer_offset;
            if (expect_read_size > 0)
            {
                size_t read_size = read_file_source_func(file_data, (char *)buffer + buffer_offset,
                                                         offset + buffer_offset, expect_read_size);
                if (read_size != expect_read_size)
                {
                    filesystem_log("Warning in general_read_func: Read size mismatch, expected: %llu, actual: %llu\n",
                                   expect_read_size, read_size);
                }
                buffer_offset = buffer_offset + read_size;
                return buffer_offset;
            }
            break;
        }
        //If the current region is cached
        if (block_id == it->first)
        {
            size_t block_offset = (offset + buffer_offset) % cache_size;
            size_t block_read_length = std::min(cache_size - block_offset, size - buffer_offset);
            if (block_read_length == 0)
                break;
            char *block_ptr = it->second;
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
            size_t read_size = read_file_source_func(file_data, (char *)buffer + buffer_offset,
                                                     offset + buffer_offset, expect_read_size);
            if (read_size != expect_read_size)
            {
                filesystem_log("Warning in general_read_func: Read size mismatch, expected: %llu, actual: %llu\n",
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
        filesystem_log("Warning in general_read_func: Final read size mismatch, expected: %llu, actual: %llu\n",
                       size, buffer_offset);
    }
    claim(buffer_offset <= size);
    return buffer_offset;
}