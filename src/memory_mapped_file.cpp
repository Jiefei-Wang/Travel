
#ifndef _WIN32
//mmap
#include <sys/mman.h>
//function close
#include <unistd.h>
//function open
#include <sys/stat.h>
#include <fcntl.h>
//error code
#include <errno.h>
#include <cstring>
#else
#include <windows.h>
#endif


#include <string>
#include <set>
#include <Rinternals.h>
#include "memory_mapped_file.h"
#include "utils.h"

//A set to keep track of the memory-mapped files
//When unmounting the filesystem, all the mapped files will be released.
std::set<Memory_mapped *> mapped_file_handle_list;

void register_file_handle(Memory_mapped *handle)
{
    mapped_file_handle_list.insert(handle);
}
void unregister_file_handle(Memory_mapped *handle)
{
    mapped_file_handle_list.erase(handle);
}
std::string unmap_all_files()
{
    filesystem_print("handle size:%d\n", mapped_file_handle_list.size());
    std::string msg;
    for (auto it = mapped_file_handle_list.begin(); it != mapped_file_handle_list.end(); ++it)
    {
        Memory_mapped *handle = *it;
        if (!handle->unmap())
        {
            msg += handle->get_last_error() + "\n";
        }
    }
    mapped_file_handle_list.clear();
    return msg;
}


size_t get_file_handle_number()
{
    return mapped_file_handle_list.size();
}

