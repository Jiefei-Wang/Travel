#ifndef HEADER_FILESYSTEM_FILE_DATA
#define HEADER_FILESYSTEM_FILE_DATA
#include <map>
#include <vector>
#include "class_Cache_block.h"
#include "class_Subset_index.h"
#include "Travel_package_types.h"

class Filesystem_cache_index_iterator;


/*
The object that holds all data of a file
member variables:
  altrep_info: The data of the source. Source has its own data type
  file_size: The file size in bytes
  unit_size: The unit size of the data in bytes.
  cache_size: The write cache size. 
  coerced_type: The R type of the file. It may be different from the source type
  index: the index of the subsetted vector for the original vector
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
    Cache_block &get_cache_block(size_t cache_id);
    size_t get_serialize_size();
    void serialize(void* ptr);
    void unserialize(void *ptr);

    Filesystem_cache_index_iterator get_cache_iterator();
public:
    Travel_altrep_info altrep_info;
    uint8_t unit_size;
    size_t file_length;
    size_t file_size;
    size_t cache_size;
    int coerced_type;
    Subset_index index;
    std::map<size_t, Cache_block> write_cache;
};

//A simple class to iterate over the index of the cached element
class Filesystem_cache_index_iterator
{
    Filesystem_file_data &file_data;
    std::map<size_t, Cache_block>::iterator block_iter;
    size_t block_start_elt;
    size_t within_block_id;
    size_t block_length;
    size_t type_size;

public:
    Filesystem_cache_index_iterator(Filesystem_file_data &file_data);
    Filesystem_cache_index_iterator &operator++();
    //get index of the element in the data
    size_t get_index();
    //get index of the element in the source data(Altrep_info)
    size_t get_index_in_source();
    bool is_final();

private:
    void compute_block_info();
};




#endif