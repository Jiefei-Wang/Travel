#define FUSE_USE_VERSION 26
#include <Rcpp.h>
#include <chrono>
#include <future>
#include <thread>
#include <shared_mutex>
#include <time.h>
#include "filesystem_manager.h"
#include "fuse_filesystem_operations.h"
#include "package_settings.h"
#include "double_key_map.h"
#include "utils.h"
#include "memory_mapped_file.h"



static inode_type file_inode_counter = 1;
std::shared_mutex filesystem_shared_mutex;
double_key_map<inode_type, std::string, filesystem_file_data> file_list;


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
  initial_filesystem_log();
  filesystem_thread.reset(new std::thread(run_filesystem_thread_func));
}
//#include <unistd.h>
// [[Rcpp::export]]
void C_stop_filesystem_thread()
{
  
  if (filesystem_thread != nullptr)
  {
    // We must release the file handle before stopping the thread
    std::string status = unmap_all_files();
    if(status!=""){
      Rf_warning(status.c_str());
    }
    //stop the filesystem
    filesystem_stop();
    //Check if the thread can be stopped
    filesystem_print("is thread ended: %s\n",thread_stopped?"TRUE":"FALSE");
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
    filesystem_log("unmount\n");
    close_filesystem_log();
  }
}

// [[Rcpp::export]]
bool is_filesystem_running()
{
  return filesystem_thread != nullptr && is_filesystem_ok();
}

// [[Rcpp::export]]
void show_thread_status(){
    Rprintf("Thread stop marker:%d\n", thread_stopped);
    Rprintf("Is filesystem thread running:%s\n",  is_filesystem_running()?"true":"false");
    Rprintf("Is filesystem ok:%s\n",  is_filesystem_ok()?"true":"false");
}



size_t fake_read(filesystem_file_data &file_data, void *buffer, size_t offset, size_t length){
    std::string data = "fake read data\n";
    for(size_t i =0;i<length;i++){
        ((char*)buffer)[i+offset] = data.c_str()[(i+offset)%(data.length())];
    }
    return length;
}

//[[Rcpp::export]]
void C_make_fake_file(size_t size)
{
    filesystem_file_data file_data;
    file_data.data_func =fake_read;
    file_data.file_size = size;
    file_data.unit_size =1;
    add_virtual_file(file_data);
}
