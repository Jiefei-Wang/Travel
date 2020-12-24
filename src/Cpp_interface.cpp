#include <Rcpp.h>
#include "Travel.h"
#include "utils.h"
#include "filesystem_manager.h"
#include "class_Protect_guard.h"
/*
=========================================================================================
                     altrep wrapper
=========================================================================================
*/
size_t read_altrep_region(const Travel_altrep_info *altrep_info, void *buffer, size_t offset, size_t length)
{
    SEXP wrapped_object = (SEXP)altrep_info->private_data;
    switch (TYPEOF(wrapped_object))
    {
    case INTSXP:
        INTEGER_GET_REGION(wrapped_object, offset, length, (int *)buffer);
        break;
    case LGLSXP:
        LOGICAL_GET_REGION(wrapped_object, offset, length, (int *)buffer);
        break;
    case REALSXP:
        REAL_GET_REGION(wrapped_object, offset, length, (double *)buffer);
        break;
    default:
        return 0;
    }

    return length;
}

//[[Rcpp::export]]
SEXP C_wrap_altrep(SEXP x)
{
    switch (TYPEOF(x))
    {
    case INTSXP:
    case LGLSXP:
    case REALSXP:
        break;
    default:
        return NULL;
    }
    Travel_altrep_info altrep_info = {};
    altrep_info.length = XLENGTH(x);
    altrep_info.private_data = x;
    altrep_info.protected_data = x;
    altrep_info.type = TYPEOF(x);
    altrep_info.operations.get_region = read_altrep_region;
    Protect_guard guard;
    SEXP altmmap_object = guard.protect(Travel_make_altrep(altrep_info));
    SHALLOW_DUPLICATE_ATTRIB(altmmap_object, x);
    return altmmap_object;
}

/*
=========================================================================================
                     debugging tools
=========================================================================================
*/
#include "altrep_manager.h"
#include "altrep_macros.h"
// [[Rcpp::export]]
void C_flush_altrep(SEXP x)
{
    flush_altrep(x);
}
// [[Rcpp::export]]
SEXP C_get_file_name(SEXP x)
{
    return get_file_name(x);
}
// [[Rcpp::export]]
SEXP C_get_file_path(SEXP x)
{
    return get_file_path(x);
}

// [[Rcpp::export]]
SEXP C_get_altmmap_cache(SEXP x)
{
    using namespace Rcpp;
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    size_t n = file_data.write_cache.size();
    Rcpp::NumericVector block_id(n);
    Rcpp::StringVector ptr(n);
    Rcpp::LogicalVector shared(n);
    size_t j = 0;
    for (const auto &i : file_data.write_cache)
    {
        block_id(j) = i.first;
        ptr(j) = std::to_string((size_t)i.second.get_const());
        shared(j) = i.second.is_shared();
        j++;
    }
    DataFrame df = DataFrame::create(Named("block.id") = block_id,
                                     Named("shared") = shared,
                                     Named("ptr") = ptr);
    return df;
}

// [[Rcpp::export]]
void C_print_cache(SEXP x, size_t i)
{
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    if (file_data.write_cache.find(i) != file_data.write_cache.end())
    {
        const double *ptr = (const double *)file_data.write_cache.find(i)->second.get_const();
        for (size_t j = 0; j < 4096 / 8; j++)
        {
            Rprintf("%f,", ptr[j]);
        }
    }
}

// [[Rcpp::export]]
SEXP C_coerce(SEXP x, int type)
{
    return Rf_coerceVector(x, type);
}

// [[Rcpp::export]]
SEXP C_getAltData1(SEXP x)
{
    return R_altrep_data1(x);
}
// [[Rcpp::export]]
SEXP C_getAltData2(SEXP x)
{
    return R_altrep_data2(x);
}

// [[Rcpp::export]]
SEXP C_get_ptr(SEXP x)
{
    return R_MakeExternalPtr(DATAPTR(x), R_NilValue, R_NilValue);
}

/*
=========================================================================================
                     For Travel_impl.h
=========================================================================================
*/
// [[Rcpp::export]]
SEXP C_call_Travel_make_altmmap(SEXP x)
{
    Travel_altrep_info *altrep_info = (Travel_altrep_info *)R_ExternalPtrAddr(x);
    return Travel_make_altmmap(*altrep_info);
}


/*========================================================================================= */
#include "memory_mapped_file.h"
// [[Rcpp::export]]
size_t C_get_file_handle_number()
{
    return get_file_handle_number();
}


/*========================================================================================= */
#include "filesystem_manager.h"

// [[Rcpp::export]]
void C_run_filesystem_thread_func()
{
    run_filesystem_thread_func();
}
// [[Rcpp::export]]
void C_run_filesystem_thread()
{
    run_filesystem_thread();
}
// [[Rcpp::export]]
void C_stop_filesystem_thread()
{
    stop_filesystem_thread();
}
// [[Rcpp::export]]
bool C_is_filesystem_running()
{
    return is_filesystem_running();
}
// [[Rcpp::export]]
void C_show_thread_status(){
    show_thread_status();
}

// [[Rcpp::export]]
void C_remove_all_filesystem_files(){
    remove_all_filesystem_files();
}

/*========================================================================================= */
#include "utils.h"
// [[Rcpp::export]]
void C_set_filesystem_log_location(Rcpp::String x)
{
    set_filesystem_log_location(x);
}
// [[Rcpp::export]]
Rcpp::String C_get_filesystem_log_location()
{
    return get_filesystem_log_location();
}
// [[Rcpp::export]]
void C_set_mountpoint(Rcpp::String path)
{
    return set_mountpoint(path);
}
// [[Rcpp::export]]
Rcpp::String C_get_mountpoint()
{
    return get_mountpoint();
}
// [[Rcpp::export]]
void C_set_debug_print(bool x)
{
	set_debug_print(x);
}
// [[Rcpp::export]]
void C_set_altrep_print(bool x)
{
	set_altrep_print(x);
}
// [[Rcpp::export]]
void C_set_filesystem_print(bool x)
{
	set_filesystem_print(x);
}
// [[Rcpp::export]]
void C_set_filesystem_log(bool x)
{
	set_filesystem_log(x);
}
// [[Rcpp::export]]
void C_initial_filesystem_log()
{
	initial_filesystem_log();
}
// [[Rcpp::export]]
void C_close_filesystem_log()
{
	close_filesystem_log();
}


/*========================================================================================= */
#include "unit_test_utils.h"
// [[Rcpp::export]]
SEXP C_make_int_sequence_altrep(double n){
    return make_int_sequence_altrep(n);
}

