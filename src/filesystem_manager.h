#ifndef HEADER_FILESYSTEM_MANAGER
#define HEADER_FILESYSTEM_MANAGER

#include <string>
#include <map>
#include "double_key_map.h"


struct filesystem_file_data;
typedef size_t (*file_data_func)(filesystem_file_data &file_data, void *buffer, size_t offset, size_t length);
struct filesystem_file_data
{
  unsigned long long file_size;
  //This function do not need to do the out-of-bound check
  //The size of an unit of the data is specified in unit_size
  file_data_func data_func;
  unsigned int unit_size;
  void *private_data;
};

struct filesystem_file_info{
    std::string file_full_path;
    std::string file_name;
    size_t file_inode;
};

#define inode_type unsigned long
extern double_key_map<inode_type, std::string, filesystem_file_data> file_list;

filesystem_file_info add_virtual_file(filesystem_file_data file_data, std::string name = std::string());
bool remove_virtual_file(std::string name);
bool is_filesystem_running();




#endif