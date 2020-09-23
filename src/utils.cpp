#define UTILS_ENABLE_R
#include "utils.h"
#include <cstdarg>
#include <iostream>
#include <fstream>
#include "package_settings.h"

bool debug_print_enabled = true;
bool altrep_print_enabled = false;
bool filesystem_print_enabled = true;
bool filesystem_log_enabled = true;

#define BUFFER_SIZE 1024 * 1024
static char buffer[BUFFER_SIZE];
static std::ofstream filesystem_log_stream;

// [[Rcpp::export]]
void initial_filesystem_log()
{
	if (filesystem_log_enabled >= 1)
	{
	filesystem_log_stream.open(get_print_location().c_str(), std::ofstream::out);
	}
}
// [[Rcpp::export]]
void close_filesystem_log()
{
	if (filesystem_log_enabled >= 1)
	{
	filesystem_log_stream.close();
	}
}

void filesystem_log(const char *format, ...)
{
	if (filesystem_log_enabled >= 1)
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
	}
	return elt_size;
}

size_t get_object_size(SEXP x)
{
	size_t elt_size = get_type_size(TYPEOF(x));
	return elt_size * XLENGTH(x);
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

std::wstring stringToWstring(const char* utf8Bytes)
{
	//setup converter
	using convert_type = std::codecvt_utf8<typename std::wstring::value_type>;
	std::wstring_convert<convert_type, typename std::wstring::value_type> converter;

	//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	return converter.from_bytes(utf8Bytes);
}
std::string wstringToString(const wchar_t* utf16Bytes)
{
	//setup converter
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;

	//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	return convert.to_bytes(utf16Bytes);
}

std::string get_file_name_in_path(std::string path) {
	return path.substr(path.find_last_of("\\/") + 1);
}
#endif