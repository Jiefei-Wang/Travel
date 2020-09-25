
#include "filesystem_manager.h"

/*
A wrapper function that will handle the request from filesystem_read and 
convert it to fit the alignment requirement specified by filesystem_file_data.unit_size
*/
size_t general_read_func(filesystem_file_data& file_data, void* buffer, size_t offset, size_t length);

void general_write_func(filesystem_file_data &file_data, const void *buffer, size_t offset, size_t size);