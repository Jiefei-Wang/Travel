
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
#else
#include <windows.h>
#endif

#include <string>
#include <set>
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
// [[Rcpp::export]]
size_t C_get_file_handle_number(){
    return mapped_file_handle_list.size();
}

#ifndef _WIN32
std::string memory_map(file_map_handle *&handle, const filesystem_file_info file_info, const size_t size)
{
    const std::string &file_full_path = file_info.file_full_path;
    //Wait until the file exist or timeout(5s)
    clock_t begin_time = clock();
    int fd = -1;
    while (fd == -1)
    {
        fd = open(file_full_path.c_str(), O_RDONLY); //read only
        if (float(clock() - begin_time) / CLOCKS_PER_SEC > FILESYSTEM_WAIT_TIME)
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
std::string memory_unmap(file_map_handle *handle)
{
    filesystem_print("releasing file handle:%s--%p\n", handle->file_info.file_name.c_str(), handle->ptr);
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
#else
std::string memory_map(file_map_handle *&handle, const filesystem_file_info file_info, const size_t size)
{
    clock_t begin_time = clock();
    HANDLE file_handle = INVALID_HANDLE_VALUE;
    while (file_handle == INVALID_HANDLE_VALUE)
    {
        file_handle = CreateFileA(file_info.file_full_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                  OPEN_EXISTING, 0, NULL);
        if (float(clock() - begin_time) / CLOCKS_PER_SEC > FILESYSTEM_WAIT_TIME)
        {
            return "Fail to open the file " + file_info.file_full_path +
                   ", error:" + std::to_string(GetLastError());
        }
    }

    HANDLE map_handle = CreateFileMappingA(file_handle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (map_handle == NULL)
    {
        CloseHandle(file_handle);
        return "Fail to map the file " + file_info.file_full_path +
               ", error:" + std::to_string(GetLastError());
    }

    void *ptr = MapViewOfFile(map_handle, FILE_MAP_READ, 0, 0, 0);
    if (ptr == NULL)
    {
        CloseHandle(map_handle);
        CloseHandle(file_handle);
        return "Fail to get a pointer from the file " + file_info.file_full_path +
               ", error:" + std::to_string(GetLastError());
    }
    handle = new file_map_handle;
    handle->file_info = file_info;
    handle->file_handle = file_handle;
    handle->map_handle = map_handle;
    handle->ptr = ptr;
    handle->size = size;
    filesystem_print("Creating file handle:%s--%p\n", handle->file_info.file_name.c_str(), handle->ptr);
    insert_mapped_file_handle(handle);
    return "";
}
std::string memory_unmap(file_map_handle *handle)
{
    filesystem_print("releasing file handle:%s--%p\n", handle->file_info.file_name.c_str(), handle->ptr);
    bool status;
    std::string msg;
    status = UnmapViewOfFile(handle->ptr);
    if (!status)
    {
        msg = "Fail to release the pointer from the file " +
              handle->file_info.file_full_path + ", error:" + std::to_string(GetLastError());
    }
    status = CloseHandle(handle->map_handle);
    if (!status && msg == "")
    {
        msg = "Fail to close the map handle for the file " +
              handle->file_info.file_full_path + ", error:" + std::to_string(GetLastError());
    }
    status = CloseHandle(handle->file_handle);
    if (!status && msg == "")
    {
        msg = "Fail to close the file handle for the file " +
              handle->file_info.file_full_path + ", error:" + std::to_string(GetLastError());
    }
    erase_mapped_file_handle(handle);
    return "";
}
#endif

std::string unmap_all_files()
{
    filesystem_print("handle size:%d\n", mapped_file_handle_list.size());
    std::set<void *> coped_set = mapped_file_handle_list;
    
    std::string msg;
    for (auto it = mapped_file_handle_list.begin(); it != mapped_file_handle_list.end();)
    {
        file_map_handle *handle = (file_map_handle *)*(it++);
        std::string status = memory_unmap(handle);
        if (status != "")
        {
            msg = status;
        }
    }
    return msg;
}