#ifndef HEADER_UTILS
#define HEADER_UTILS

#include "Travel_types.h"
#define TRAVEL_PACKAGE_DEBUG

#ifdef TRAVEL_PACKAGE_DEBUG
#include <assert.h> 
#define claim(x) assert(x)
#else
#define claim(x)
#endif



#ifdef _WIN32
#undef ERROR
#endif

#ifdef UTILS_ENABLE_R
#include <Rcpp.h>
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


void initial_filesystem_log();
void close_filesystem_log();
void filesystem_log(const char *format, ...);
void debug_print(const char *format, ...);
void filesystem_print(const char *format, ...);
void altrep_print(const char *format, ...);
size_t get_type_size(int type);
void mySleep(int sleepMs);

size_t gcd(size_t a, size_t b);
//Least Common Multiple
size_t lcm(size_t a, size_t b);
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
#ifdef _WIN32
std::wstring stringToWstring(const char *utf8Bytes);
std::string wstringToString(const wchar_t *utf16Bytes);
std::wstring stringToWstring(std::string utf8Bytes);
std::string wstringToString(std::wstring utf16Bytes);
#endif

#endif