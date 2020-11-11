#ifndef HEADER_MEMORY_MAPPED_FILE
#define HEADER_MEMORY_MAPPED_FILE
#include <string>
#include "filesystem_manager.h"

struct file_map_handle
{
    file_map_handle(
                    std::string file_path,
                    void *file_handle,
                    void *map_handle,
                    void *ptr,
                    size_t size,
                    bool filesystem_file) : file_path(file_path),
                                   file_handle(file_handle),
                                   map_handle(map_handle),
                                   ptr(ptr),
                                   size(size),
                                   filesystem_file(filesystem_file) {}
    std::string file_path;
    void *file_handle;
    void *map_handle;
    void *ptr;
    size_t size;
    bool filesystem_file;
};

/*
Map/Unmap files

If success, the file handle will be returned by file_map_handle
If not success, the error message will be returned

Args:
  handle: the file handle
  file_path: path to the file
  size: size of the file
  filesystem_file: is the file from the Travel filesystem?
*/
std::string memory_map(file_map_handle *&handle, std::string file_path, const size_t size, bool filesystem_file = true);
std::string memory_unmap(file_map_handle *handle);
std::string flush_handle(file_map_handle *handle);

bool has_mapped_file_handle(void *handle);
std::string unmap_filesystem_files();

#endif