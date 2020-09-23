#define FUSE_USE_VERSION 26
#include <Rcpp.h>
#include <chrono>
#include <future>
#include <thread>
#include <shared_mutex>
#include <time.h>
#include "filesystem_manager.h"
#include "filesystem_operations.h"
#include "package_settings.h"
#include "double_key_map.h"
#include "utils.h"
#include "memory_mapped_file.h"

static inode_type file_inode_counter = 1;
double_key_map<inode_type, std::string, filesystem_file_data> file_list;

/*
==========================================================================
Insert or delete files from the filesystem
==========================================================================
*/

filesystem_file_info add_virtual_file(filesystem_file_data file_data, std::string name)
{
  if(file_data.file_size%file_data.unit_size!=0){
    Rf_error("The file size and unit size does not match!\n");
  }
  if(file_data.file_size<file_data.unit_size){
    Rf_error("The file size is less than unit size!\n");
  }
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
struct thread_guard
{
  thread_guard(bool &thread) : thread_finished(thread)
  {
    thread_finished = false;
  }
  ~thread_guard()
  {
    thread_finished = true;
  }

private:
  bool &thread_finished;
};
static std::unique_ptr<std::thread> filesystem_thread(nullptr);
//An indicator to check whether the thread is running or not.
//When a thread reaches the end of its excution this indicator will becomes true.
static bool thread_finished = true;
// [[Rcpp::export]]
void run_filesystem_thread_func()
{
  thread_guard guard(thread_finished);
  filesystem_thread_func();
}

// [[Rcpp::export]]
void C_run_filesystem_thread()
{
  if (filesystem_thread != nullptr)
  {
    Rf_error("The filesystem thread has been running!\n");
  }
  if(get_mountpoint()==""){
    Rf_error("The mount point have not been set!\n");
  }
  initial_filesystem_log();
  filesystem_thread.reset(new std::thread(run_filesystem_thread_func));
  clock_t begin_time = clock();
  while (thread_finished||!is_filesystem_alive())
    {
      if (float(clock() - begin_time) / CLOCKS_PER_SEC > FILESYSTEM_WAIT_TIME)
      {
        Rf_warning("The filesystem may not be started successfully!\n");
        return;
      }
    }
}
//#include <unistd.h>
// [[Rcpp::export]]
void C_stop_filesystem_thread()
{
  if (filesystem_thread != nullptr)
  {
    // We must release the file handle before stopping the thread
    std::string status = unmap_all_files();
    if (status != "")
    {
      Rf_warning(status.c_str());
    }
    mySleep(1);
    //stop the filesystem
    filesystem_stop();
    //Check if the thread can be stopped
    filesystem_print("is thread ended: %s\n", thread_finished ? "TRUE" : "FALSE");
    clock_t begin_time = clock();
    while (!thread_finished)
    {
      if (float(clock() - begin_time) / CLOCKS_PER_SEC > FILESYSTEM_WAIT_TIME)
      {
        Rf_warning("The thread cannot be stopped for the filesystem is still busy\n");
        return;
      }
    }
    filesystem_print("is thread ended: %s\n", thread_finished ? "TRUE" : "FALSE");
    filesystem_thread->join();
    filesystem_thread.reset(nullptr);
    filesystem_log("unmount\n");
    close_filesystem_log();
  }
}

// [[Rcpp::export]]
bool C_is_filesystem_running(){
  return is_filesystem_running();
}

bool is_filesystem_running()
{
  if(thread_finished&&(filesystem_thread != nullptr)){
     C_stop_filesystem_thread();
  }
  return filesystem_thread != nullptr && is_filesystem_alive();
}

// [[Rcpp::export]]
void show_thread_status()
{
  Rprintf("Thread stop indicator:%d\n", thread_finished);
  Rprintf("Is filesystem thread running:%s\n", is_filesystem_running() ? "true" : "false");
  Rprintf("Is filesystem ok:%s\n", is_filesystem_alive() ? "true" : "false");
}
