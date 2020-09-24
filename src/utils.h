#ifndef HEADER_UTILS
#define HEADER_UTILS

#define DEBUG_ALTREP(x)
#define DEBUG_ALTPTR(x) x

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

#include <time.h>
class Timer{
  private:
  clock_t begin_time;
    int time;
    public:
  Timer(int time):time(time){
    begin_time = clock();
  }
  bool expired(){
    return float(clock() - begin_time) / CLOCKS_PER_SEC > time;
  }
  void reset(){
    begin_time = clock();
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
size_t get_read_size(size_t file_size, size_t offset, size_t size);

#ifdef _WIN32
std::wstring stringToWstring(const char *utf8Bytes);
std::string wstringToString(const wchar_t *utf16Bytes);
std::wstring stringToWstring(std::string utf8Bytes);
std::string wstringToString(std::wstring utf16Bytes);
std::string get_file_name_in_path(std::string path);
#endif

#endif