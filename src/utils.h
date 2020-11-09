#ifndef HEADER_UTILS
#define HEADER_UTILS

#define TRAVEL_PACKAGE_DEBUG

#ifdef TRAVEL_PACKAGE_DEBUG
#include <assert.h>
#define claim(x) assert(x)
#else
#define claim(x)
#endif

//math macro
#define round_up_division(x, y) (x / y + (x % y != 0))
#define zero_bounded_minus(x, y) (x > y ? x - y : 0)

#ifdef _WIN32
#undef ERROR
#endif

#ifdef UTILS_ENABLE_R
#ifndef R_INTERNALS_H_
#include "Rcpp.h"
#endif
size_t get_object_size(SEXP x);
class PROTECT_GUARD
{
private:
  int protect_num = 0;

public:
  PROTECT_GUARD() {}
  ~PROTECT_GUARD()
  {
    if (protect_num != 0)
      UNPROTECT(protect_num);
  }
  SEXP protect(SEXP x)
  {
    protect_num++;
    return PROTECT(x);
  }
};
#endif

#include <chrono>
class Timer
{
private:
  std::chrono::time_point<std::chrono::steady_clock> begin_time;
  int time;

public:
  Timer(int time) : time(time)
  {
    begin_time = std::chrono::steady_clock::now();
  }
  bool expired()
  {
    auto end_time = std::chrono::steady_clock::now();
    double elapsed_time_s = std::chrono::duration<double>(end_time - begin_time).count();
    return elapsed_time_s > time;
  }
  void reset()
  {
    begin_time = std::chrono::steady_clock::now();
  }
};

#include <memory>
class Unique_buffer
{
  size_t size = 0;
  std::unique_ptr<char[]> ptr;
public:
  void reserve(size_t reserve_size)
  {
    if (reserve_size > size)
    {
      ptr.reset(new char[reserve_size]);
      size = reserve_size;
    }
  }
  void release()
  {
    if (size > 1024 * 1024)
    {
      ptr.reset(nullptr);
      size = 0;
    }
  }
  char* get(){
    return ptr.get();
  }
};

void initial_filesystem_log();
void close_filesystem_log();
void filesystem_log(const char *format, ...);
void debug_print(const char *format, ...);
void filesystem_print(const char *format, ...);
void altrep_print(const char *format, ...);
uint8_t get_type_size(int type);
std::string get_type_name(int type);
void mySleep(int sleepMs);
size_t get_file_read_size(size_t file_size, size_t offset, size_t size);

/*
Examples:
build_path("", "bucket") = "bucket"
build_path("bucket", "") = "bucket"
build_path("bucket", "/") = "bucket/"
build_path("bucket", "test") = "bucket/test"
build_path("bucket/", "test") = "bucket/test"
build_path("bucket/", "/test") = "bucket/test"
*/
std::string build_path(std::string path1, std::string path2);
std::string get_file_name_in_path(std::string path);
/*Copy data between two pointer with different types*/
void copy_memory(int dest_type, int src_type, void *dest, const void *src, size_t length, bool reverse = false);

#ifdef _WIN32
std::wstring stringToWstring(const char *utf8Bytes);
std::string wstringToString(const wchar_t *utf16Bytes);
std::wstring stringToWstring(std::string utf8Bytes);
std::string wstringToString(std::wstring utf16Bytes);
#endif

#endif