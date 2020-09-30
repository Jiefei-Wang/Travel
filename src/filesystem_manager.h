#ifndef HEADER_FILESYSTEM_MANAGER
#define HEADER_FILESYSTEM_MANAGER

#include <string>
#include "Travel_types.h"

filesystem_file_info add_virtual_file(file_data_func data_func,
                                      void *private_data,
                                      size_t file_size,
                                      unsigned int unit_size = 1,
                                      const char *name = NULL);
filesystem_file_data& get_virtual_file(std::string name);
bool remove_virtual_file(std::string name);
bool is_filesystem_running();

#endif