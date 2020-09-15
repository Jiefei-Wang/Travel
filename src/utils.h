#include <Rcpp.h>
#define DEBUG_ALTREP(x)
#define DEBUG_ALTPTR(x) x

void print_to_file(const char* format, ...);


void debug_print(const char* format, ...);

size_t get_object_size(SEXP x);
size_t get_type_size(int type);


void mySleep(int sleepMs);


class PROTECT_GUARD{
  private:
    int protect_num = 0;
  public:
    PROTECT_GUARD(){}
    ~PROTECT_GUARD(){
      if(protect_num!=0)
        UNPROTECT(protect_num);
    }
    SEXP protect(SEXP x){
      protect_num ++;
      return PROTECT(x);
    }
};
