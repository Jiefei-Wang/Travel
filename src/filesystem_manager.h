#ifndef HEADER_FILESYSTEM_MANAGER
#define HEADER_FILESYSTEM_MANAGER

#include <string>
#include "Travel_package_types.h"

/*
A struct that holds File name, full path and inode number
*/
struct filesystem_file_info
{
    std::string file_full_path;
    std::string file_name;
    size_t file_inode;
};

class Cache_block
{
private:
    size_t size;
    size_t *counter = nullptr;
    char *ptr = nullptr;

public:
    Cache_block(size_t size);
    ~Cache_block();
    Cache_block(const Cache_block &cb);
    Cache_block(Cache_block &cb);
    Cache_block &operator=(const Cache_block &cb);
    size_t use_count() const;
    bool is_shared() const;
    size_t get_size() const;
    char *get();
    const char *get_const() const;
};

/*
The struct that holds all data of a file
member variables:
  altrep_info: The data of the source. Source has its own data type
  file_size: The file size in bytes
  unit_size: The unit size of the data in bytes.
  cache_size: The write cache size. 
  coerced_type: The type of the file. It may be different from the source type
  write_cache: All the changes to the data will be stored here by block. It
    should be stored in the format of the coerced type.
*/
struct Filesystem_file_data
{
    Filesystem_file_data(int type, const Travel_altrep_info altrep_info);
    Travel_altrep_info altrep_info;
    size_t file_size;
    size_t cache_size;
    int coerced_type;
    std::map<size_t, Cache_block> write_cache;
};

/*
Manage virtual files
*/
filesystem_file_info add_virtual_file(int type,
                                      Travel_altrep_info altrep_info,
                                      const char *name = NULL);
const std::string &get_virtual_file_name(inode_type inode);
inode_type get_virtual_file_inode(const std::string name);
Filesystem_file_data &get_virtual_file(const std::string name);
Filesystem_file_data &get_virtual_file(inode_type inode);
bool has_virtual_file(const std::string name);
bool has_virtual_file(inode_type inode);
bool remove_virtual_file(const std::string name);
typename std::map<const inode_type, const std::string>::iterator virtual_file_begin();
typename std::map<const inode_type, const std::string>::iterator virtual_file_end();

bool is_filesystem_running();
#endif