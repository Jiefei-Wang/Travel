#define FUSE_USE_VERSION 26
#include <Rcpp.h>
#include <chrono>
#include <future>
#include <thread>
#include <shared_mutex>
#include "filesystem_manager.h"
#include "filesystem_operations.h"
#include "package_settings.h"
#include "double_key_map.h"
#include "utils.h"
#include "memory_mapped_file.h"

static inode_type file_inode_counter = 1;
double_key_map<inode_type, std::string, filesystem_file_data> file_list;

filesystem_file_data::filesystem_file_data(file_data_func data_func,
                                           void *private_data,
                                           size_t file_size,
                                           unsigned int unit_size) : data_func(data_func),
                                                                     private_data(private_data),
                                                                     file_size(file_size),
                                                                     unit_size(unit_size)
{
  cache_size = lcm(MIN_CACHE_SIZE, unit_size);
}

/*
==========================================================================
Insert or delete files from the filesystem
==========================================================================
*/

filesystem_file_info add_virtual_file(file_data_func data_func,
                                      void *private_data,
                                      size_t file_size,
                                      unsigned int unit_size,
                                      const char *name)
{
  if (file_size % unit_size != 0)
  {
    Rf_error("The file size and unit size does not match!\n");
  }
  file_inode_counter++;
  std::string file_name;
  if (name == NULL)
    file_name = "inode_" + std::to_string(file_inode_counter);
  else
    file_name = std::string(name);
  filesystem_file_data file_data(data_func, private_data, file_size, unit_size);
  file_list.insert(file_inode_counter, file_name, file_data);
  std::string file_full_path = build_path(get_mountpoint(), file_name);
  return {file_full_path, file_name, file_inode_counter};
}

bool remove_virtual_file(std::string name)
{
  if (file_list.has_key2(name))
  {
    filesystem_file_data &file_data = file_list.get_value_by_key2(name);
    for(auto i:file_data.write_cache){
      delete i.second;
    }
    return file_list.erase_value_by_key2(name);
  }
  return false;
}

// [[Rcpp::export]]
Rcpp::DataFrame C_list_virtual_files()
{
  using namespace Rcpp;
  int n = file_list.size();
  CharacterVector name(n);
  NumericVector inode(n);
  NumericVector unit_size(n);
  NumericVector file_size(n);
  NumericVector cache_size(n);
  NumericVector cache_number(n);
  int j = 0;
  for (auto i = file_list.begin_key(); i != file_list.end_key(); i++)
  {
    name[j] = i->second;
    inode[j] = i->first;
    filesystem_file_data &file_data = file_list.get_value_by_key1(i->first);
    unit_size[j] = file_data.unit_size;
    file_size[j] = file_data.file_size;
    cache_size[j] = file_data.cache_size;
    cache_number[j] = file_data.write_cache.size();
    j++;
  }
  DataFrame df = DataFrame::create(Named("name") = name,
                                   Named("inode") = inode,
                                   Named("unit size") = unit_size,
                                   Named("file size") = file_size,
                                   Named("cache size") = cache_size,
                                   Named("cache number") = cache_number);
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
#define THREAD_INIT INT_MAX
static int thread_status;
// [[Rcpp::export]]
void run_filesystem_thread_func()
{
  thread_guard guard(thread_finished);
  filesystem_thread_func(&thread_status);
}

void C_stop_filesystem_thread();
// [[Rcpp::export]]
void C_run_filesystem_thread()
{
  if (filesystem_thread != nullptr)
  {
    Rf_error("The filesystem thread has been running!\n");
  }
  if (get_mountpoint() == "")
  {
    Rf_error("The mount point have not been set!\n");
  }
  thread_status = THREAD_INIT;
  initial_filesystem_log();
  filesystem_thread.reset(new std::thread(run_filesystem_thread_func));
  //Check if the thread is running correctly, if not, kill the thread
  mySleep(100);
  if (thread_status != THREAD_INIT)
  {
    Rf_warning("The filesystem has been stopped, killing the filesystem thread\n");
    C_stop_filesystem_thread();
    return;
  }
  Timer timer(FILESYSTEM_WAIT_TIME);
  while (!is_filesystem_alive())
  {
    if (timer.expired())
    {
      Rf_warning("The filesystem may not start correctly\n");
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
    mySleep(100);
    //stop the filesystem
    filesystem_stop();
    //Check if the thread can be stopped
    filesystem_print("is thread ended: %s\n", thread_finished ? "TRUE" : "FALSE");
    Timer timer(FILESYSTEM_WAIT_TIME);
    while (!thread_finished)
    {
      if (timer.expired())
      {
        Rf_warning("The thread cannot be stopped for the filesystem is still busy\n");
        return;
      }
    }
    filesystem_print("is thread ended: %s\n", thread_finished ? "TRUE" : "FALSE");
    filesystem_thread->join();
    filesystem_thread.reset(nullptr);
    if (thread_status != 0)
    {
      Rf_warning("The filesystem did not end correctly, error code:%d(%s)",
                 thread_status,
                 get_error_message(thread_status).c_str());
    }
    close_filesystem_log();
  }
}

// [[Rcpp::export]]
bool C_is_filesystem_running()
{
  return is_filesystem_running();
}

bool is_filesystem_running()
{
  if (thread_finished && (filesystem_thread != nullptr))
  {
    C_stop_filesystem_thread();
  }
  return filesystem_thread != nullptr && is_filesystem_alive();
}

// [[Rcpp::export]]
void show_thread_status()
{
  Rprintf("Thread stop indicator:%d\n", thread_finished);
  Rprintf("Is filesystem thread running:%s\n", filesystem_thread != nullptr ? "true" : "false");
  Rprintf("Is filesystem alive:%s\n", is_filesystem_alive() ? "true" : "false");
}
