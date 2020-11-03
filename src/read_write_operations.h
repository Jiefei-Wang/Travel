
#ifndef HEADER_READ_WRITE_OPERATION
#define HEADER_READ_WRITE_OPERATION
#include "filesystem_manager.h"

size_t get_file_read_size(size_t file_size, size_t offset, size_t size);
/*
A wrapper function that will handle the request from filesystem_read and 
convert it to fit the alignment requirement specified by Filesystem_file_data.unit_size
*/
size_t general_read_func(Filesystem_file_data& file_data, void* buffer, size_t offset, size_t size);

size_t general_write_func(Filesystem_file_data &file_data, const void *buffer, size_t offset, size_t size);

#endif