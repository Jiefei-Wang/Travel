#ifndef HEADER_TRAVEL_TYPES
#define HEADER_TRAVEL_TYPES
#include <string>
#include <map>
#include <memory>

#define inode_type unsigned long

struct filesystem_file_data;
/*
The function that reads the data from the file,
this function do not need to do the out-of-bound check.
Args: 
  file_data: The file info
  buffer: The buffer where the data will be written to
  offset: The offset(index) of the vector.
  length: The length of the data.
*/
typedef size_t (*file_data_func)(filesystem_file_data &file_data, void *buffer, size_t offset, size_t length);

/*
The struct that holds all data of a file
member variables:
  data_func: The function to read the data from the file
  private_data: A pointer that can be used to store some private data of the file
  file_size: The file size in bytes
  unit_size: The unit size of the data in bytes. The offset and length
             that are passed to the function data_func will be calculated based on this unit.
             DO NOT TRY TO CHANGE IT!
  cache_size: The write cache size. DO NOT TRY TO CHANGE IT!
  write_cache: All the changes to the data will be stored here by block. DO NOT TRY TO CHANGE IT!
*/
struct filesystem_file_data
{
  filesystem_file_data(file_data_func data_func,
                       void *private_data,
                       size_t file_size,
                       unsigned int unit_size = 1);
  file_data_func data_func;
  void *private_data;
  size_t file_size;
  unsigned int unit_size;
  size_t cache_size;
  std::map<size_t, char*> write_cache;
};

/*
A struct that holds File name, full path and inode number
*/
struct filesystem_file_info
{
  std::string file_full_path;
  std::string file_name;
  size_t file_inode;
};
#endif