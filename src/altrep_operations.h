#include <Rcpp.h>
#include <R_ext/Altrep.h>
#include "filesystem_manager.h"


R_altrep_class_t getAltClass(int type);

SEXP make_altptr(int type, void *data, size_t length, unsigned int unit_size, file_data_func read_func, SEXP protect = R_NilValue);