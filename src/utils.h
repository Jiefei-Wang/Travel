#ifndef HEADER_UTILS
#define HEADER_UTILS

#define STRINGIZING(x) #x
#define XSTR(x) STRINGIZING(x)

#include <stdexcept>
#define throw_msg(x) throw std::runtime_error("The condition <" XSTR(x) "> Does not meet at line number "\
 XSTR(__LINE__)" in file <" __FILE__">")
#define throw_if_not(x) if(!(x))throw_msg(x)
#define throw_if(x) if(x)throw_msg(x)
#define throw_error(msg) throw std::runtime_error(msg)


//math macro
#define round_up_division(x, y) (x / y + (x % y != 0))
#define zero_bounded_minus(x, y) (x > y ? x - y : 0)



/*
=============================
        filesystem path
=============================
*/
void set_filesystem_log_location(std::string x);
std::string get_filesystem_log_location();
void set_mountpoint(std::string path);
std::string get_mountpoint();
//Initial log system
void initial_filesystem_log();
void close_filesystem_log();
//switch print
void set_debug_print(bool x);
void set_altrep_print(bool x);
void set_filesystem_print(bool x);
void set_filesystem_log(bool x);
//Print method
void filesystem_log(const char *format, ...);
void debug_print(const char *format, ...);
void filesystem_print(const char *format, ...);
void altrep_print(const char *format, ...);


#ifdef Rcpp_hpp
uint64_t get_object_size(SEXP x);
#endif
uint8_t get_type_size(int type);
std::string get_type_name(int type);
void sleep(int sleepMs);
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
void covert_data(int dest_type, int src_type, void *dest, const void *src, size_t length, bool reverse = false);

#ifdef _WIN32
std::wstring stringToWstring(const char *utf8Bytes);
std::string wstringToString(const wchar_t *utf16Bytes);
std::wstring stringToWstring(std::string utf8Bytes);
std::string wstringToString(std::wstring utf16Bytes);
#endif

#endif