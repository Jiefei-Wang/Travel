#include "filesystem_manager.h"
#include "Travel.h"
#include "utils.h"

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
SEXP C_make_altmmap_from_altrep(SEXP x)
{
    Travel_altrep_info altrep_info = {};
    altrep_info.length = XLENGTH(x);
    altrep_info.private_data = x;
    altrep_info.protected_data = x;
    altrep_info.type = TYPEOF(x);
    altrep_info.operations.get_region = read_altrep_region;
    PROTECT_GUARD guard;
    SEXP altmmap_object = guard.protect(Travel_make_altrep(altrep_info));
    SHALLOW_DUPLICATE_ATTRIB(altmmap_object, x);
    return altmmap_object;
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
void print_value(SEXP x)
{
    void *ptr = DATAPTR(x);
    if (ptr == NULL || ptr == nullptr)
    {
        Rf_error("The pointer is NULL!\n");
    }
    int max_print = (XLENGTH(x) > 100 ? 100 : XLENGTH(x));
    for (int i = 0; i < max_print; i++)
    {
        switch (TYPEOF(x))
        {
        case INTSXP:
            Rprintf("%d,", ((int *)ptr)[i]);
            break;
        case REALSXP:
            Rprintf("%f,", ((double *)ptr)[i]);
            break;
        default:
            break;
        }
    }
    Rprintf("\n");
}

// [[Rcpp::export]]
SEXP C_get_ptr(SEXP x)
{
    return R_MakeExternalPtr(DATAPTR(x), R_NilValue, R_NilValue);
}

/*
=========================================================================================
                     altmmap related operations
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
double mySum1(SEXP x)
{
    double s = 0;
    R_xlen_t len = XLENGTH(x);
    for (R_xlen_t i = 0; i < len; i++)
    {
        s += REAL_ELT(x, i);
    }
    return s;
}

#include "R_ext/Itermacros.h"
// [[Rcpp::export]]
double mySum2(SEXP x)
{
    double s = 0.0;
    ITERATE_BY_REGION(x, ptr, ind, nbatch, double, REAL,
                      {
                          for (int i = 0; i < nbatch; i++)
                          {
                              s = s + ptr[i];
                          }
                      });
    return s;
}


// [[Rcpp::export]]
double mySum3(SEXP x)
{
    double s = 0.0;
    double* ptr = (double*)DATAPTR(x);
    R_xlen_t len = XLENGTH(x);
    for (R_xlen_t i = 0; i < len; i++)
    {
        s += ptr[i];
    }
    return s;
}



#include "Travel.h"
size_t arithmetic_sequence_region(const Travel_altrep_info *altrep_info, void *buffer,
                                     size_t offset, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        ((double *)buffer)[i] = offset + i + 1;
    }
    return length;
}

// [[Rcpp::export]]
SEXP Travel_compact_seq(size_t n)
{
    Travel_altrep_info altrep_info;
    altrep_info.length = n;
    altrep_info.type = REALSXP;
    altrep_info.operations.get_region = arithmetic_sequence_region;
    SEXP x = Travel_make_altrep(altrep_info);
    return x;
}

