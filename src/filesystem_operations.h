#ifndef HEADER_FILESYSTEM_OPERATIONS
#define HEADER_FILESYSTEM_OPERATIONS
/*
The main function that is responsible for
mounting a filesystem and handling all
file access requests from R. The function will 
be excuted in a new thread. It should block the 
thread until filesystem_stop is called.
*/
void filesystem_thread_func();
/*
The function to stop the filesystem,
it should unmount the filesystem and unblock
the thread that is running the function filesystem_thread_func.

This function will be called in main thread.
*/
void filesystem_stop();
/*
Whether the filesystem is running or not.
If fail to unmount a filesystem due to some reason,
this function can return true to prevent a second mount
and let the caller to call filesystem_stop later.
*/
bool is_filesystem_alive();

/*
An utility to get the true read size that will not read out-of-bound
*/
inline size_t get_read_size(size_t file_size, size_t offset, size_t size){
    if (offset + size > file_size)
    {
        if (offset >= file_size)
        {
            return 0;
        }
        else
        {
            return file_size - offset;
        }
    }else{
        return size;
    }
}



#endif