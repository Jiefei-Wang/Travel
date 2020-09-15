
#include <string>
#include "filesystem_manager.h"

struct file_map_handle
{
    filesystem_file_info file_info;
    void* file_handle;
    void* map_handle;
    void *ptr;
    size_t size;
};

std::string memory_map(file_map_handle *&handle, const filesystem_file_info file_key, const size_t size);
std::string memory_unmap(file_map_handle *handle);

bool has_mapped_file_handle(void* handle);
void unmap_all_files();

