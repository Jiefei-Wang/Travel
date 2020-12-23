#ifndef HEADER_MEMORY_MAPPED_FILE
#define HEADER_MEMORY_MAPPED_FILE
#include <string>
#include "class_Memory_mapped.h"

void register_file_handle(Memory_mapped *handle);
void unregister_file_handle(Memory_mapped *handle);
std::string unmap_all_files();
size_t get_file_handle_number();

#endif
