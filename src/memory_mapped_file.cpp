#include <Rcpp.h>
#include <string>
//mmap
#include <sys/mman.h>
//function close
#include <unistd.h>
//function open
#include <sys/stat.h>
#include <fcntl.h>
//error code
#include <errno.h>
#include <time.h>
#include "package_settings.h"
#include "filesystem_manager.h"
#include "memory_mapped_file.h"
#include "utils.h"


//All the memory-mapped files should be stored here
//When unmounting the filesystem, all the mapped files will be released.
std::set<void *> mapped_file_handle_list;

/*
==========================================================================
Function to manager the handles of the memory-mapped file
==========================================================================
*/
void insert_mapped_file_handle(void *handle)
{
  mapped_file_handle_list.insert(handle);
}
void erase_mapped_file_handle(void *handle)
{
  mapped_file_handle_list.erase(handle);
}
bool has_mapped_file_handle(void *handle)
{
  return mapped_file_handle_list.find(handle) != mapped_file_handle_list.end();
}



std::string memory_map_linux(file_map_handle *&handle, const filesystem_file_info file_info, const size_t size)
{
    const std::string &file_full_path = file_info.file_full_path;
    //Wait until the file exist or timeout(5s)
    clock_t begin_time = clock();
    int fd = -1;
    while (float(clock() - begin_time) / CLOCKS_PER_SEC < 5)
    {
        fd = open(file_full_path.c_str(), O_RDONLY); //read only
        if (fd != -1)
            break;
    }
    if (fd == -1)
    {
        return "Fail to open the file %s" + file_full_path + ", error: " + std::to_string(errno) + "\n";
    }
    void *ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == (void *)-1)
    {
        close(fd);
        return "Fail to map the file %s" + file_full_path + ", error: " + std::to_string(errno) + "\n";
    }
    close(fd);
    handle = new file_map_handle;
    handle->file_info = file_info;
    handle->ptr = ptr;
    handle->size = size;
    insert_mapped_file_handle(handle);
    return "";
}
std::string memory_map(file_map_handle *&handle, const filesystem_file_info file_info, const size_t size)
{
    return memory_map_linux(handle, file_info, size);
}

std::string memory_unmap_linux(file_map_handle *handle)
{
    if (!has_mapped_file_handle(handle))
    {
        return "The handle has been released";
    }
    int status = munmap(
        handle->ptr,
        handle->size);
    erase_mapped_file_handle(handle);
    delete handle;
    if (status == -1)
        return "Fail to unmap the file" + handle->file_info.file_name + ", error: " + std::to_string(errno) + "\n";
    else
        return "";
}

std::string memory_unmap(file_map_handle *handle)
{
    return memory_unmap_linux(handle);
}


void unmap_all_files(){
    debug_print("handle size:%d\n", mapped_file_handle_list.size());
    std::set<void *> coped_set = mapped_file_handle_list;
    for (auto i : coped_set)
    {
      file_map_handle* handle = (file_map_handle*)i;
      debug_print("releasing:%s--%p\n",handle->file_info.file_name.c_str(),handle->ptr);
      std::string status = memory_unmap(handle);
      if (status != "")
      {
        Rf_warning(status.c_str());
      }
    }
}