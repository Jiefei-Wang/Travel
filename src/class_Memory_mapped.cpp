
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
#include <Rinternals.h>
#include "class_Memory_mapped.h"
#include "utils.h"
#include "class_Timer.h"

Memory_mapped::Memory_mapped(std::string file_path, const size_t size, Cache_hint hint) : file_path(file_path), size(size), hint(hint)
{
    map();
}
Memory_mapped::~Memory_mapped()
{
    bool status = unmap();
    if (!status)
    {
        Rf_warning(error_msg.c_str());
    }
}
#ifndef _WIN32
bool Memory_mapped::map()
{
    if(mapped)
    {
        return true;
    }
    //Wait until the file exist or timeout(5s)
    Timer timer(FILESYSTEM_WAIT_TIME);
    int fd = -1;
    while(fd == -1)
    {
        fd = open(file_path.c_str(), O_RDWR);
        if (timer.expired())
            break;
    }
    if (fd == -1)
    {
         error_msg = "Fail to open the file " + file_path + ", error: " + strerror(errno) + "\n";
         return false;
    }
    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if(ptr == (void *)-1)
    {
        ptr=nullptr;
        error_msg = "Fail to map the file " + file_path + ", error: " + strerror(errno) + "\n";
        return false;
    }
    mapped = true;
    return true;
}
bool Memory_mapped::unmap()
{
    if (!mapped)
        return true;
    filesystem_print("releasing file handle:%s--%p\n", file_path.c_str(), ptr);
    int status = munmap(
        ptr,
        size);
    if (status == -1){
        error_msg= "Fail to unmap the file" + file_path + ", error: " + strerror(errno) + "\n";
        return false;
    }
    else{
        return true;
    }
}
bool Memory_mapped::flush()
{
    if(!mapped)
    {
        return true;
    }
    int status = msync(ptr, size, MS_SYNC);
    if (status != 0)
    {
        error_msg = "Fail to flush the changes to the memory mapped file" +
                    file_path + ", error: " + strerror(errno) + "\n";
        return false;
    }
    return true;
}
#else
bool Memory_mapped::map()
{
    if (mapped)
    {
        return true;
    }
    DWORD file_attributes = 0;
    switch (hint)
    {
    case Normal:
        file_attributes = FILE_ATTRIBUTE_NORMAL;
        break;
    case SequentialScan:
        file_attributes = FILE_FLAG_SEQUENTIAL_SCAN;
        break;
    case RandomAccess:
        file_attributes = FILE_FLAG_RANDOM_ACCESS;
        break;
    default:
        break;
    }

    Timer timer(file_wait_time);
    file_handle = INVALID_HANDLE_VALUE;
    while (file_handle == INVALID_HANDLE_VALUE)
    {
        file_handle = CreateFileA(file_path.c_str(),
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                  OPEN_EXISTING, file_attributes, NULL);
        if (timer.expired())
        {
            error_msg = "Fail to open the file " + file_path +
                        ", error:" + std::to_string(GetLastError()) + "\n";
            return false;
        }
    }

    map_handle = CreateFileMappingA(file_handle, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (map_handle == NULL)
    {
        CloseHandle(file_handle);
        file_handle = INVALID_HANDLE_VALUE;
        error_msg = "Fail to map the file " + file_path +
                    ", error:" + std::to_string(GetLastError()) + "\n";
        return false;
    }
    ptr = MapViewOfFile(map_handle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (ptr == NULL)
    {
        CloseHandle(map_handle);
        CloseHandle(file_handle);
        file_handle = INVALID_HANDLE_VALUE;
        map_handle = NULL;
        error_msg = "Fail to get a pointer from the file " + file_path +
                    ", error:" + std::to_string(GetLastError()) + "\n";
        ptr = nullptr;
        return false;
    }
    mapped = true;
    filesystem_print("Creating file handle:%s--%p\n", file_path.c_str(), ptr);
    return true;
}
bool Memory_mapped::unmap()
{
    if (!mapped)
        return true;
    filesystem_print("releasing file handle:%s--%p\n", file_path.c_str(), ptr);
    bool status;
    error_msg = "";
    if (ptr != nullptr)
    {
        status = UnmapViewOfFile(ptr);
        if (!status)
        {
            error_msg = "Fail to release the pointer from the file " +
                        file_path + ", error:" + std::to_string(GetLastError()) + "\n";
            return false;
        }
        else
        {
            ptr = nullptr;
        }
    }
    if (map_handle != NULL)
    {
        status = CloseHandle(map_handle);
        if (!status)
        {
            error_msg = "Fail to close the map handle for the file " +
                        file_path + ", error:" + std::to_string(GetLastError()) + "\n";
            return false;
        }
        else
        {
            map_handle = NULL;
        }
    }
    if (file_handle != INVALID_HANDLE_VALUE)
    {
        status = CloseHandle(file_handle);
        if (!status)
        {
            error_msg = "Fail to close the file handle for the file " +
                        file_path + ", error:" + std::to_string(GetLastError()) + "\n";
            return false;
        }
        else
        {
            file_handle = INVALID_HANDLE_VALUE;
        }
    }
    if (error_msg == "")
    {
        mapped = false;
        return true;
    }
    else
    {
        return false;
    }
}
bool Memory_mapped::flush()
{
    if (!mapped)
    {
        return true;
    }
    bool status = FlushViewOfFile(ptr, 0);
    if (!status)
    {
        error_msg = "Fail to flush the view of the file " + file_path +
                    ", error:" + std::to_string(GetLastError()) + "\n";
        return false;
    }
    status = FlushFileBuffers(file_handle);
    if (!status)
    {
        error_msg = "Fail to flush the file buffer " + file_path +
                    ", error:" + std::to_string(GetLastError()) + "\n";
        return false;
    }
    return true;
}
#endif
