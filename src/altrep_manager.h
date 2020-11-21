#ifndef HEADER_ALTREP_MANAGER
#define HEADER_ALTREP_MANAGER

#ifndef R_INTERNALS_H_
#include <Rcpp.h>
#endif

struct Filesystem_file_identifier;
struct Travel_altrep_info;


SEXP Travel_make_altmmap(Filesystem_file_identifier& file_info);
SEXP Travel_make_altmmap(Travel_altrep_info& altrep_info);
//This is an alias for Travel_make_altmmap
SEXP Travel_make_altrep(Travel_altrep_info altrep_info);

SEXP make_altmmap_from_file(std::string path, int type, size_t length);

SEXP get_file_name(SEXP x);
SEXP get_file_path(SEXP x);
void flush_altrep(SEXP x);
#endif