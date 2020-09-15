#include <memory>
#include <Rcpp.h>
#include "read_operations.h"
#include "altrep_operations.h"
#include "utils.h"

size_t general_read_func(filesystem_file_data &file_data, void *buffer, size_t offset, size_t size)
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

