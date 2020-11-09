#ifndef HEADER_FILESYSTEM_MANAGER
#define HEADER_FILESYSTEM_MANAGER

#include <string>
#include "class_Subset_index.h"
#include "class_Filesystem_file_data.h"
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
bool remove_filesystem_file(inode_type inode);
typename std::map<const inode_type, const std::string>::iterator filesystem_file_begin();
typename std::map<const inode_type, const std::string>::iterator filesystem_file_end();

bool is_filesystem_running();
#endif