#include "filesystem_manager.h"
#include <memory>
class Filesystem_cache_copier
{
    Filesystem_file_data &dest_file_info;
    Filesystem_file_data &source_file_info;
    size_t source_cache_id = 0;
    const char *source_cache_ptr = nullptr;
    size_t dest_cache_id = 0;
    char * dest_cache_ptr;
public:
    Filesystem_cache_copier(Filesystem_file_data &dest_file_info, Filesystem_file_data &source_file_info);
    ~Filesystem_cache_copier();
    void copy(size_t dest_index, size_t source_index);
    void load_source_cache(size_t source_data_offset);
    void load_dest_cache(size_t dest_data_offset);
    void flush_buffer();
};