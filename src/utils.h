#include <Rcpp.h>
#define DEBUG_ALTREP(x) x

void debug_print(const char* format, ...);



size_t get_object_size(SEXP x);
size_t get_type_size(int type);

template<class T>
T mod(T a, T b)
{
   if(b < 0) //you can check for b == 0 separately and do what you want
     return -mod(-a, -b);   
   T ret = a % b;
   if(ret < 0)
     ret+=b;
   return ret;
}