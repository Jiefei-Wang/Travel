
#include <Rcpp.h>
#include "utils.h"
#include "filesystem_manager.h"
#include "read_write_operations.h"
#include "altrep_macros.h"
//#include "gperftools/profiler.h"

static int altmmap_get_int_elt(SEXP x, R_xlen_t i){
    altrep_print("Accessing elt at %llu\n", (unsigned long long)i);
    int result;
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    general_read_func(file_data, &result, i*sizeof(result), sizeof(result));
    return result;
}
static double altmmap_get_numeric_elt(SEXP x, R_xlen_t i){
    altrep_print("Accessing elt at %llu\n", (unsigned long long)i);
    double result;
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    general_read_func(file_data, &result, i*sizeof(result), sizeof(result));
    return result;
}

// [[Rcpp::export]]
double profile_int(SEXP x)
{
    //ProfilerStart("/home/jiefei/Documents/test/tmp/dump.txt");
    double s = 0;
    R_xlen_t len = XLENGTH(x);
    for (R_xlen_t i = 0; i < len; i++)
    {
        s += altmmap_get_int_elt(x, i);
    }
        //ProfilerFlush();
       // ProfilerStop();
    return s;
}

// [[Rcpp::export]]
double profile_int2(SEXP x)
{
    double s = 0;
    R_xlen_t len = XLENGTH(x);
    for (R_xlen_t i = 0; i < len; i++)
    {
        s += INTEGER_ELT(x,i);
    }
    return s;
}

// [[Rcpp::export]]
double profile_int3(SEXP x)
{
    double s = 0;
    int* ptr = (int*)DATAPTR(x);
    R_xlen_t len = XLENGTH(x);
    for (R_xlen_t i = 0; i < len; i++)
    {
        s += ptr[i];
    }
    return s;
}

// [[Rcpp::export]]
double profile_num(SEXP x)
{
    double s = 0;
    R_xlen_t len = XLENGTH(x);
    for (R_xlen_t i = 0; i < len; i++)
    {
        s += altmmap_get_numeric_elt(x, i);
    }
    return s;
}

// [[Rcpp::export]]
double profile_num2(SEXP x)
{
    double s = 0;
    R_xlen_t len = XLENGTH(x);
    for (R_xlen_t i = 0; i < len; i++)
    {
        s += REAL_ELT(x,i);
    }
    return s;
}