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
double_key_map<inode_type, std::string, Filesystem_file_data> file_list;



Filesystem_file_info make_file_info(std::string& file_name, inode_type& inode){
    std::string file_full_path = build_path(get_mountpoint(), file_name);
    return {file_full_path, file_name, inode};
}

/*
==========================================================================
Insert or delete files from the filesystem
==========================================================================
*/
Filesystem_file_info add_filesystem_file(const int type,
                                         const Subset_index &index,
                                         const Travel_altrep_info &altrep_info,
                                         const char *name)
{
  if (altrep_info.type == 0)
  {
    Rf_error("Unspecified vector type!\n");
  }
  if (altrep_info.operations.get_region == NULL)
  {
    Rf_error("The function <get_region> is NULL!\n");
  }
  inode_type file_inode = file_inode_counter++;
  std::string file_name;
  if (name == NULL)
    file_name = "inode_" + std::to_string(file_inode);
  else
    file_name = std::string(name);
  Filesystem_file_data file_data(type, index, altrep_info);
  file_list.insert(file_inode, file_name, file_data);
  Filesystem_file_info file_info = make_file_info(file_name, file_inode);
  return file_info;
}
std::string get_filesystem_file_path(std::string file_name){
      return build_path(get_mountpoint(), file_name);
}
std::string get_filesystem_file_path(inode_type inode){
      return build_path(get_mountpoint(), get_filesystem_file_name(inode));
}

const std::string &get_filesystem_file_name(inode_type inode)
{
  return file_list.get_key2(inode);
}
inode_type get_filesystem_file_inode(std::string name)
{
  return file_list.get_key1(name);
}
Filesystem_file_data &get_filesystem_file_data(std::string name)
{
  return file_list.get_value_by_key2(name);
}
Filesystem_file_data &get_filesystem_file_data(inode_type inode)
{
  return file_list.get_value_by_key1(inode);
}

bool has_filesystem_file(std::string name)
{
  return file_list.has_key2(name);
}
bool has_filesystem_file(inode_type inode)
{
  return file_list.has_key1(inode);
}
bool remove_filesystem_file(std::string name)
{
  if (has_filesystem_file(name))
  {
    return file_list.erase_value_by_key2(name);
  }
  return false;
}
bool remove_filesystem_file(inode_type inode)
{
  if (has_filesystem_file(inode))
  {
    return file_list.erase_value_by_key1(inode);
  }
  return false;
}
typename std::map<const inode_type, const std::string>::iterator filesystem_file_begin()
{
  return file_list.begin_key();
}
typename std::map<const inode_type, const std::string>::iterator filesystem_file_end()
{
  return file_list.end_key();
}
// [[Rcpp::export]]
Rcpp::DataFrame C_get_virtual_file_list()
{
  using namespace Rcpp;
  int n = file_list.size();
  CharacterVector name(n);
  NumericVector inode(n);
  NumericVector file_size(n);
  NumericVector cache_size(n);
  NumericVector cache_number(n);
  NumericVector shared_cache_number(n);
  int j = 0;
  for (auto i = file_list.begin_key(); i != file_list.end_key(); i++)
  {
    name[j] = i->second;
    inode[j] = i->first;
    Filesystem_file_data &file_data = file_list.get_value_by_key1(i->first);
    file_size[j] = file_data.file_size;
    cache_size[j] = file_data.cache_size;
    cache_number[j] = file_data.write_cache.size();
    size_t count = 0;
    for (const auto &it : file_data.write_cache)
    {
      count = count + it.second.is_shared();
    }
    shared_cache_number[j] = count;
    j++;
  }
  DataFrame df = DataFrame::create(Named("name") = name,
                                   Named("inode") = inode,
                                   Named("file.size") = file_size,
                                   Named("cache.size") = cache_size,
                                   Named("cache.number") = cache_number,
                                   Named("shared.cache.number") = shared_cache_number);
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
    std::string status = unmap_filesystem_files();
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
