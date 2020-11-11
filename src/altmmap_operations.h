#ifndef HEADER_ALTMMAP_OPERATIONS
#define HEADER_ALTMMAP_OPERATIONS

#ifndef R_INTERNALS_H_
#include <Rcpp.h>
#endif
#include <R_ext/Altrep.h>

R_altrep_class_t get_altmmap_class(int type);
bool is_altmmap(SEXP x);

#endif