#define UTILS_ENABLE_R
#include "utils.h"
#include <cstdarg>
#include <iostream>
#include <fstream>
#include "package_settings.h"

bool debug_print_enabled = false;
bool altrep_print_enabled = false;
bool filesystem_print_enabled = false;
bool filesystem_log_enabled = false;

#define BUFFER_SIZE 1024 * 1024
static char buffer[BUFFER_SIZE];
static std::ofstream filesystem_log_stream;
static bool filesystem_log_opened = false;

// [[Rcpp::export]]
void C_set_debug_print(bool x){
	debug_print_enabled = x;
}
// [[Rcpp::export]]
void C_set_altrep_print(bool x){
	altrep_print_enabled = x;
}
// [[Rcpp::export]]
void C_set_filesystem_print(bool x){
	filesystem_print_enabled = x;
}
// [[Rcpp::export]]
void C_set_filesystem_log(bool x){
	filesystem_log_enabled = x;
}

// [[Rcpp::export]]
void initial_filesystem_log()
{
	if (filesystem_log_enabled && !filesystem_log_opened)
	{
		filesystem_log_stream.open(get_print_location().c_str(), std::ofstream::out);
		filesystem_log_opened = true;
	}
}
// [[Rcpp::export]]
void close_filesystem_log()
{
	if (filesystem_log_enabled && filesystem_log_opened)
	{
		filesystem_log_stream.close();
		filesystem_log_opened = false;
	}
}


void filesystem_log(const char *format, ...)
{
	if (filesystem_log_enabled)
	{
		//initial_filesystem_log();
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, BUFFER_SIZE, format, args);
		filesystem_log_stream << buffer;
		filesystem_log_stream.flush();
	}
}

void debug_print(const char *format, ...)
{
	if (debug_print_enabled)
	{
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, BUFFER_SIZE, format, args);
		Rprintf(buffer);
	}
}

void filesystem_print(const char *format, ...)
{
	if (filesystem_print_enabled)
	{
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, BUFFER_SIZE, format, args);
		Rprintf(buffer);
	}
}

void altrep_print(const char *format, ...)
{
	if (altrep_print_enabled)
	{
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, BUFFER_SIZE, format, args);
		Rprintf(buffer);
	}
}

size_t get_type_size(int type)
{
	size_t elt_size = 0;
	switch (type)
	{
	case INTSXP:
		elt_size = sizeof(int);
		break;
	case LGLSXP:
		elt_size = sizeof(int);
		break;
	case REALSXP:
		elt_size = sizeof(double);
		break;
	case RAWSXP:
		elt_size = 1;
		break;
	}
	return elt_size;
}

size_t get_object_size(SEXP x)
{
	size_t elt_size = get_type_size(TYPEOF(x));
	return elt_size * XLENGTH(x);
}


std::string to_linux_slash(std::string path)
{
	for (size_t i = 0; i < path.size(); ++i)
	{
		if (path.at(i) == '\\')
		{
			path.replace(i, 1, "/");
		}
	}
	return path;
}
std::string build_path(std::string path1, std::string path2)
{
	path1 = to_linux_slash(path1);
	path2 = to_linux_slash(path2);
	char delimiter = '/';
	if (path1.length() == 0)
	{
		return path2;
	}
	if (path2.length() == 0)
	{
		return path1;
	}
	if (path1.at(path1.length() - 1) == delimiter &&
		path2.at(0) == delimiter)
	{
		path2 = path2.erase(0);
	}
	if (path1.length() == 0 ||
		path2.length() == 0 ||
		path1.at(path1.length() - 1) == delimiter ||
		path2.at(0) == delimiter)
	{
		return path1 + path2;
	}
	else
	{
		return path1 + delimiter + path2;
	}
}

size_t gcd(size_t a, size_t b)
{
	if (a == 0)
		return b;
	return gcd(b % a, a);
}

// Function to return LCM of two numbers
size_t lcm(size_t a, size_t b)
{
	return (a * b) / gcd(a, b);
}

#ifndef _WIN32
#include <unistd.h>
void mySleep(int sleepMs)
{
	usleep(sleepMs * 1000); // usleep takes sleep time in us (1 millionth of a second)
}
#else

#undef Realloc
#undef Free
#include <windows.h>
#include <codecvt>
#include <locale>

void mySleep(int sleepMs)
{
	Sleep(sleepMs);
}

std::wstring stringToWstring(std::string utf8Bytes)
{
	return stringToWstring(utf8Bytes.c_str());
}
std::string wstringToString(std::wstring utf16Bytes)
{
	return wstringToString(utf16Bytes.c_str());
}

std::wstring stringToWstring(const char *utf8Bytes)
{
	//setup converter
	using convert_type = std::codecvt_utf8<typename std::wstring::value_type>;
	std::wstring_convert<convert_type, typename std::wstring::value_type> converter;

	//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	return converter.from_bytes(utf8Bytes);
}
std::string wstringToString(const wchar_t *utf16Bytes)
{
	//setup converter
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;

	//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	return convert.to_bytes(utf16Bytes);
}

std::string get_file_name_in_path(std::string path)
{
	return path.substr(path.find_last_of("\\/") + 1);
}
#endif