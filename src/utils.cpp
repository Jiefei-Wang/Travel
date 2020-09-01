#include "utils.h"
#include <mutex>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <string>

using std::string;

int verbose_level = 1;

#define BUFFER_SIZE 1024 * 1024
static bool print_file_initialed = false;
static string print_location;
static std::mutex output_mutex;
static char buffer[BUFFER_SIZE];
static std::ofstream print_file;


// [[Rcpp::export]]
void set_print_location(SEXP x){
	print_location = CHAR(Rf_asChar(x));
	print_location = print_location + "/debug_output";
}
// [[Rcpp::export]]
SEXP get_print_location(){
	return Rf_mkString(print_location.c_str());
}
// [[Rcpp::export]]
void initial_print_file(){
	if(!print_file_initialed){
  		print_file.open (print_location.c_str(),std::ios_base::openmode::_S_out);
		print_file_initialed = true;
	}
}
// [[Rcpp::export]]
void close_print_file(){
	print_file.close();
	print_file_initialed =false;
}

// [[Rcpp::export]]
void test_print(){
	debug_print("This is a test print");
	print_file.flush();
}

void debug_print(const char* format, ...) {
    if(verbose_level >= 1){
		initial_print_file();
        output_mutex.lock();
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, BUFFER_SIZE, format, args);
		print_file << buffer;
		print_file.flush();
		output_mutex.unlock();
    }
}

size_t get_type_size(int type){
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

size_t get_object_size(SEXP x){
	size_t elt_size = get_type_size(TYPEOF(x));
	return elt_size * XLENGTH(x);
}


int mod (int a, int b)
{
   if(b < 0) //you can check for b == 0 separately and do what you want
     return -mod(-a, -b);   
   int ret = a % b;
   if(ret < 0)
     ret+=b;
   return ret;
}