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
    size_t final_read_size = 0;
    char *cbuffer = (char *)buffer;
    std::unique_ptr<char[]> ptr(new char[unit_size]);
    //Fill the begin of the data if needed
    size_t start_offset = offset / unit_size;
    size_t start_intra_elt_offset = offset % unit_size;
    if (start_intra_elt_offset != 0)
    {
        size_t read_size = file_data.data_func(file_data, ptr.get(), start_offset, 1);
        if (read_size != 1)
            return final_read_size;
        size_t copy_size = std::min(size, unit_size - start_intra_elt_offset);
        memcpy(cbuffer, ptr.get() + start_intra_elt_offset, copy_size);
        cbuffer = cbuffer + copy_size;
        offset = offset + copy_size;
        size = size - copy_size;
        final_read_size = final_read_size + copy_size;
        start_offset++;
    }
    if (size == 0)
    {
        return final_read_size;
    }
    //Fill the middle of the data
    bool has_middle_data = true;
    size_t aligned_end_offset;
    size_t end_offset = (offset + size - 1) / unit_size;
    size_t end_intra_elt_len = (offset + size) % unit_size;
    if (end_intra_elt_len != 0)
    {
        if (end_offset == start_offset)
        {
            has_middle_data = false;
        }
        else
        {
            aligned_end_offset = end_offset - 1;
        }
    }
    else
    {
        aligned_end_offset = end_offset;
    }

    //Start filling the middle data
    if (has_middle_data)
    {
        size_t length = aligned_end_offset + 1 - start_offset;
        size_t read_length = file_data.data_func(file_data, cbuffer, start_offset, length);
        final_read_size = final_read_size + read_length * unit_size;
        if (read_length != length)
            return final_read_size;
        cbuffer = cbuffer + read_length * unit_size;
    }
    //Fill the end of the data
    if (end_intra_elt_len != 0)
    {
        size_t read_length = file_data.data_func(file_data, ptr.get(), end_offset, 1);
        if (read_length != 1)
            return final_read_size;
        memcpy(cbuffer, ptr.get(), end_intra_elt_len);
        final_read_size = final_read_size + unit_size;
    }
    return final_read_size;
}
/*
    Read the data from source
*/
size_t read_file_source_func(filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    return read_local_file_func(file_data, buffer, offset, size);
}



void general_write_func(filesystem_file_data &file_data, const void *buffer, size_t offset, size_t size)
{
    size_t &cache_size = file_data.cache_size;
    size_t buffer_offset = 0;
    while (buffer_offset < size)
    {
        size_t block_id = (offset + buffer_offset) / cache_size;
        size_t block_offset = (offset + buffer_offset) % cache_size;
        size_t block_write_length = cache_size - block_offset;
        if (size - buffer_offset < block_write_length)
        {
            block_write_length = size - buffer_offset;
        }

        if (file_data.write_cache.find(block_id)==file_data.write_cache.end())
        {
            filesystem_log("Creating new block %llu\n",block_id);
            file_data.write_cache[block_id] = new char[cache_size];
            if(block_offset!=0||block_write_length!=cache_size){
                size_t read_size = get_valid_file_size(file_data.file_size,block_offset*cache_size,cache_size);
                read_file_source_func(file_data,file_data.write_cache[block_id],
                block_offset*cache_size,read_size);
            }
        }
        char *block_ptr = file_data.write_cache[block_id];
        memcpy(block_ptr + block_offset, (char *)buffer + buffer_offset, block_write_length);
        buffer_offset = buffer_offset + block_write_length;
    }
}


size_t general_read_func(filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
{
    size_t &cache_size = file_data.cache_size;
    size_t lowest_block_id = offset / cache_size;
    size_t highest_block_id = (offset + size - 1) / cache_size;
    //Find the iterator that points to the block whose id is not less than lowest_block_id
    auto it = file_data.write_cache.lower_bound(lowest_block_id);
    //The data that has been read
    size_t buffer_offset = 0;
    while (buffer_offset < size)
    {
        size_t block_id = (offset + buffer_offset) / cache_size;
        //If no cached block exists or the block id exceeds highest_block_id
        if (it == file_data.write_cache.end() || it->first > highest_block_id)
        {
            size_t read_size = read_file_source_func(file_data, (char *)buffer + buffer_offset,
                                                       offset + buffer_offset, size - buffer_offset);
            buffer_offset = buffer_offset + read_size;
            break;
        }
        //If the current region is cached
        if (block_id == it->first)
        {
            size_t block_offset = (offset + buffer_offset) % cache_size;
            size_t block_read_length = cache_size - block_offset;
            if (size - buffer_offset < block_read_length)
            {
                block_read_length = size - buffer_offset;
            }
            char *block_ptr = it->second;
            memcpy((char *)buffer + buffer_offset, block_ptr + block_offset, block_read_length);
            buffer_offset = buffer_offset + block_read_length;
            ++it;
        }
        else if (block_id < it->first)
        {
            //If the current region is not cached but
            //there exists other cached block

            size_t next_block_offset = it->first * cache_size;
            size_t required_read_size = next_block_offset - (offset + buffer_offset);
            size_t read_size = read_file_source_func(file_data, (char *)buffer + buffer_offset,
                                                       offset + buffer_offset, required_read_size);
            buffer_offset = buffer_offset + read_size;
            if (read_size != required_read_size)
            {
                break;
            }
        }
    }
    return buffer_offset;
}