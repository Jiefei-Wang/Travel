#include <Rcpp.h>
#include <string>

std::string add_altrep_to_fuse(SEXP x, SEXP name);
void remove_altrep_from_fuse(SEXP name);