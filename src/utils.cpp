#include "utils.h"
#include <mutex>
#include <cstdarg>
#include <iostream>
#include <fstream>

int verbose_level = 1;

#define BUFFER_SIZE 1024 * 1024
static std::mutex output_mutex;
static char buffer[BUFFER_SIZE];

void debug_print(const char* format, ...) {
    if(verbose_level > 1){
        output_mutex.lock();
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, BUFFER_SIZE, format, args);
		std::ofstream myfile;
  		myfile.open ("debug_output.txt");
		myfile << buffer;
 		myfile.close();
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