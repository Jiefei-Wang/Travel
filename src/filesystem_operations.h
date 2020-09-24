#ifndef HEADER_FILESYSTEM_OPERATIONS
#define HEADER_FILESYSTEM_OPERATIONS
/*
The main function that is responsible for
mounting a filesystem and handling all
file access requests from R. The function will 
be excuted in a new thread. It should block the 
thread until filesystem_stop is called.

Args:
    thread_status: The return value of the filesystem
*/
void filesystem_thread_func(int* thread_status);
/*
The function to stop the filesystem,
it should unmount the filesystem and unblock
the thread that is running the function filesystem_thread_func.

This function will be called in main thread and should be safe
to be called even without filesystem running.
*/
void filesystem_stop();
/*
Whether the filesystem is running or not.
If fail to unmount a filesystem due to some reason,
this function can return true to prevent a second mount
and let the caller to call filesystem_stop later.
*/
bool is_filesystem_alive();

std::string get_error_message(int status);



#endif