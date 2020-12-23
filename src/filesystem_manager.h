#ifndef HEADER_FILESYSTEM_MANAGER
#define HEADER_FILESYSTEM_MANAGER
#include <string>
#include "class_Subset_index.h"
#include "class_Filesystem_file_data.h"
#include "Travel_package_types.h"
/*
A struct that holds File name, full path and inode number
*/
struct Filesystem_file_identifier
{
    std::string file_full_path;
    std::string file_name;
    size_t file_inode;
};

/*
==========================================================
Manage virtual files
==========================================================
*/
Filesystem_file_identifier add_filesystem_file(const int type,
                                         const Subset_index &index,
                                         const Travel_altrep_info &altrep_info,
                                         const char *name = NULL);
//Get the file name, inode and path
std::string get_filesystem_file_path(inode_type inode);
std::string get_filesystem_file_path(std::string name);
const std::string &get_filesystem_file_name(inode_type inode);
inode_type get_filesystem_file_inode(std::string name);
//Get file data
Filesystem_file_data &get_filesystem_file_data(std::string name);
Filesystem_file_data &get_filesystem_file_data(inode_type inode);
//Check the existance of the file
bool has_filesystem_file(std::string name);
bool has_filesystem_file(inode_type inode);
//Delete the file from the filesystem
bool remove_filesystem_file(std::string name);
bool remove_filesystem_file(inode_type inode);
//File iterator
typename std::map<const inode_type, const std::string>::iterator filesystem_file_begin();
typename std::map<const inode_type, const std::string>::iterator filesystem_file_end();





/*
==========================================================
Filesystem management
==========================================================
*/
void run_filesystem_thread_func();
void run_filesystem_thread();
void stop_filesystem_thread();
bool is_filesystem_running();
void show_thread_status();
#endif