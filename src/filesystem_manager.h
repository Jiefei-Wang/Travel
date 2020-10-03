#ifndef HEADER_FILESYSTEM_MANAGER
#define HEADER_FILESYSTEM_MANAGER

#include <string>
#include "Travel_types.h"

/*
A struct that holds File name, full path and inode number
*/
struct filesystem_file_info
{
  std::string file_full_path;
  std::string file_name;
  size_t file_inode;
};


/*
The struct that holds all data of a file
member variables:
  data_func: The function to read the data from the file
  file_size: The file size in bytes
  unit_size: The unit size of the data in bytes. The offset and length
             that are passed to the function data_func will be calculated based on this unit.
             DO NOT TRY TO CHANGE IT!
  cache_size: The write cache size. DO NOT TRY TO CHANGE IT!
  write_cache: All the changes to the data will be stored here by block. DO NOT TRY TO CHANGE IT!
*/
struct filesystem_file_data
{
  filesystem_file_data(Travel_altrep_info altrep_info,
                       size_t file_size);
  Travel_altrep_info altrep_info;
  size_t file_size;
  size_t cache_size;
  std::map<size_t, char *> write_cache;
};


filesystem_file_info add_virtual_file(Travel_altrep_info altrep_info,
                                      size_t file_size,
                                      const char *name = NULL);
const std::string& get_virtual_file_name(inode_type inode);
inode_type get_virtual_file_inode(const std::string name);
filesystem_file_data& get_virtual_file(const std::string name);
filesystem_file_data& get_virtual_file(inode_type inode);
bool has_virtual_file(const std::string name);
bool has_virtual_file(inode_type inode);
bool remove_virtual_file(const std::string name);
typename std::map<const inode_type,const std::string>::iterator virtual_file_begin();
typename std::map<const inode_type,const std::string>::iterator virtual_file_end();


bool is_filesystem_running();
#endif