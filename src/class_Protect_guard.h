#ifndef HEADER_PROTECT_GUARD
#define HEADER_PROTECT_GUARD
#include <Rcpp.h>

class Protect_guard
{
private:
  int protect_num;
public:
  Protect_guard();
  ~Protect_guard();
  SEXP protect(SEXP x);
};
#endif