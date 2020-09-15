
#include "filesystem_manager.h"

/*
A wrapper function that will handle the request from filesystem_read and 
convert it to fit the unit size requirement specified in filesystem_file_data.unit_size
*/
size_t general_read_func(filesystem_file_data& file_data, void* buffer, size_t offset, size_t length);


//size_t read_altrep(filesystem_file_data& file_data, void* buffer, size_t offset, size_t size);
