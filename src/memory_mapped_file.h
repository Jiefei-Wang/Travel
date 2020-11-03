#ifndef HEADER_MEMORY_MAPPED_FILE
#define HEADER_MEMORY_MAPPED_FILE
#include <string>
#include "filesystem_manager.h"


struct file_map_handle
{
    file_map_handle(Filesystem_file_info file_info,
                    void *file_handle,
                    void *map_handle,
                    void *ptr,
                    size_t size) : file_info(file_info),
                                   file_handle(file_handle),
                                   map_handle(map_handle),
                                   ptr(ptr),
                                   size(size) {}
    Filesystem_file_info file_info;
    void *file_handle;
    void *map_handle;
    void *ptr;
    size_t size;
};

std::string memory_map(file_map_handle *&handle, const Filesystem_file_info file_key, const size_t size, bool filesystem_handle = true);
std::string memory_unmap(file_map_handle *handle, bool filesystem_handle = true);
std::string flush_handle(file_map_handle *handle);

bool has_mapped_file_handle(void *handle);
std::string unmap_all_files();

#endif