
#ifndef HEADER_READ_WRITE_OPERATION
#define HEADER_READ_WRITE_OPERATION
#include "filesystem_manager.h"

size_t get_file_read_size(size_t file_size, size_t offset, size_t size);
void* load_cache(Filesystem_file_data &file_data, size_t cache_id);
/*
Functions to read/write data from/to the file
*/
size_t general_read_func(Filesystem_file_data& file_data, void* buffer, size_t offset, size_t size);

size_t general_write_func(Filesystem_file_data &file_data, const void *buffer, size_t offset, size_t size);


#endif