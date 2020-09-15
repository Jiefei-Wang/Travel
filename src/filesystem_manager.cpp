#define FUSE_USE_VERSION 26
#include <Rcpp.h>
#include <chrono>
#include <future>
#include <thread>
#include <fuse/fuse_lowlevel.h>
#include <shared_mutex>
#include <set>
#include <time.h>
#include "filesystem_manager.h"
#include "fuse_filesystem_operations.h"
#include "package_settings.h"
#include "double_key_map.h"
#include "utils.h"
#include "memory_mapped_file.h"

static fuse_ino_t file_inode_counter = 1;
extern std::shared_mutex filesystem_shared_mutex;
double_key_map<fuse_ino_t, std::string, filesystem_file_data> file_list;


/*
==========================================================================
Insert or delete files from the filesystem
==========================================================================
*/
filesystem_file_info add_virtual_file(filesystem_file_data file_data, std::string name)
{
  std::lock_guard<std::shared_mutex> guard(filesystem_shared_mutex);
  file_inode_counter++;
  if (name == "")
  {
    name = "inode_" + std::to_string(file_inode_counter);
  }
  file_list.insert(file_inode_counter, name, file_data);
  std::string file_full_path(get_mountpoint() + "/" + name);

  return {file_full_path, name, file_inode_counter};
}

bool remove_virtual_file(std::string name)
{
  std::lock_guard<std::shared_mutex> guard(filesystem_shared_mutex);
  return file_list.erase_value_by_key2(name);
}

// [[Rcpp::export]]
Rcpp::DataFrame C_list_virtual_files()
{
  using namespace Rcpp;
  int n = file_list.size();
  CharacterVector name(n);
  NumericVector inode(n);
  NumericVector file_size(n);
  int j = 0;
  for (auto i = file_list.begin_key(); i != file_list.end_key(); i++)
  {
    name[j] = i->second;
    inode[j] = i->first;
    filesystem_file_data &file_data = file_list.get_value_by_key1(i->first);
    file_size[j] = file_data.file_size;
    j++;
  }
  DataFrame df = DataFrame::create(Named("name") = name,
                                   Named("inode") = inode,
                                   Named("size") = file_size);
  return df;
}


/*
==========================================================================
Run or stop the filesystem
==========================================================================
*/
struct thread_guard {
  thread_guard(bool& thread):thread_stopped(thread){
    thread_stopped = false;
  }
  ~thread_guard() { 
    thread_stopped = true; 
  }
private:
  bool& thread_stopped;
};
static std::unique_ptr<std::thread> filesystem_thread(nullptr);
static bool thread_stopped = true;
// [[Rcpp::export]]
void run_filesystem_thread_func()
{
  thread_guard guard(thread_stopped);
  filesystem_thread_func();
}

// [[Rcpp::export]]
void C_run_filesystem_thread()
{
  if (filesystem_thread != nullptr)
  {
    Rf_error("The filesystem thread has been running!");
  }
  filesystem_thread.reset(new std::thread(run_filesystem_thread_func));
}
//#include <unistd.h>
// [[Rcpp::export]]
void C_stop_filesystem_thread()
{
  
  if (filesystem_thread != nullptr)
  {
    // We must release the file handle before stopping the thread
    unmap_all_files();
    //stop the filesystem
    filesystem_stop();
    //Check if the thread can be stopped
    debug_print("thread:%d\n",(int)thread_stopped);
    clock_t begin_time = clock();
    while (!thread_stopped)
    {
      if (float(clock() - begin_time) / CLOCKS_PER_SEC > 3)
      {
        Rf_warning("The thread may not be stopped, the filesystem is still busy\n");
        return;
      }
    }
    filesystem_thread->join();
    filesystem_thread.reset(nullptr);
    print_to_file("unmount\n");
  }
}

// [[Rcpp::export]]
bool is_filesystem_running()
{
  return filesystem_thread != nullptr && is_filesystem_alive();
}

// [[Rcpp::export]]
void show_thread_status(){
    Rprintf("Is thread stopped:%d\n", thread_stopped);
    Rprintf("Is filesystem alive:%d\n",  is_filesystem_alive());
}