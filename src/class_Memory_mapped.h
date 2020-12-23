#ifndef HEADER_MEMORY_MAPPED
#define HEADER_MEMORY_MAPPED
#include <string>

class Memory_mapped
{
public:
    enum Cache_hint
    {
        Normal,         ///< good overall performance
        SequentialScan, ///< read file only once with few seeks
        RandomAccess    ///< jump around
    };

private:
    static const size_t file_wait_time = 5;
    std::string file_path;
    size_t size;
    Cache_hint hint = Normal;
#ifndef _WIN32
    void *file_handle = nullptr;
    void *map_handle = nullptr;
#else
    void *file_handle;
    void *map_handle = NULL;
#endif
    void *ptr = nullptr;
    bool mapped = false;
    std::string error_msg;

public:
    Memory_mapped(std::string file_path, const size_t size, Cache_hint hint = Normal);
    ~Memory_mapped();
    bool map();
    bool unmap();
    bool flush();
    void* get_ptr(){
        return ptr;
    }
    bool is_mapped()
    {
        return mapped;
    }
    size_t get_size()
    {
        return size;
    }
    std::string get_last_error()
    {
        return error_msg;
    }
    std::string get_file_path()
    {
        return file_path;
    }
    
};

#endif