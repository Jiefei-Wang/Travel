#ifndef HEADER_FILESYSTEM_MANAGER
#define HEADER_FILESYSTEM_MANAGER

#include <string>
#include "Travel_package_types.h"

/*
A struct that holds File name, full path and inode number
*/
struct Filesystem_file_info
{
    std::string file_full_path;
    std::string file_name;
    size_t file_inode;
};

class Subset_index
{
public:
    size_t start = 0;
    size_t step = 1;
    //The length of each block
    size_t block_length = 1;
    //Is the subset index consecutive?
    bool is_consecutive() const;
    /*
    Get the index of the element in the source 
    from the index of the element in the subset
    */
    size_t get_source_index(size_t i) const;
    /*
    Get the index of the element in the subset 
    from the index of the element in the source
    */
    size_t get_subset_index(size_t i) const;
    //Length of the subset vector
    size_t get_length(size_t source_length) const;
};
/*
A copy-on-write shared pointer implementation
*/
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
The object that holds all data of a file
member variables:
  altrep_info: The data of the source. Source has its own data type
  file_size: The file size in bytes
  unit_size: The unit size of the data in bytes.
  cache_size: The write cache size. 
  coerced_type: The R type of the file. It may be different from the source type
  subset_index: the index of the subsetted vector for the original vector
  write_cache: All the changes to the data will be stored here by block. It
    should be stored in the format of the coerced type.
*/
class Filesystem_file_data
{
public:
    Filesystem_file_data(int coerced_type,
                         const Subset_index &index,
                         const Travel_altrep_info &altrep_info);
    size_t get_cache_id(size_t data_offset);
    size_t get_cache_offset(size_t cache_id);
    size_t get_cache_size(size_t cache_id);
    bool has_cache_id(size_t cache_id);
    Cache_block& get_cache_block(size_t cache_id);

public:
    Travel_altrep_info altrep_info;
    uint8_t unit_size;
    size_t file_length;
    size_t file_size;
    size_t cache_size;
    int coerced_type;
    Subset_index subset_index;
    std::map<size_t, Cache_block> write_cache;
};

/*
Manage virtual files
*/
Filesystem_file_info add_filesystem_file(const int type,
                                         const Subset_index &index,
                                         const Travel_altrep_info &altrep_info,
                                         const char *name = NULL);
const std::string &get_filesystem_file_name(inode_type inode);
inode_type get_filesystem_file_inode(const std::string name);
Filesystem_file_data &get_filesystem_file_data(const std::string name);
Filesystem_file_data &get_filesystem_file_data(inode_type inode);
bool has_filesystem_file(const std::string name);
bool has_filesystem_file(inode_type inode);
bool remove_filesystem_file(const std::string name);
typename std::map<const inode_type, const std::string>::iterator filesystem_file_begin();
typename std::map<const inode_type, const std::string>::iterator filesystem_file_end();

bool is_filesystem_running();
#endif