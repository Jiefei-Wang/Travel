#include "Travel.h"
#include "utils.h"
#include "filesystem_manager.h"

size_t read_altrep(Travel_altrep_info* altrep_info, void *buffer, size_t offset, size_t length)
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

//Travel_make_altptr(int type, void *data, size_t length, unsigned int unit_size, read_data_func read_func, SEXP protect)
//[[Rcpp::export]]
SEXP C_make_altptr_from_altrep(SEXP x)
{
    Travel_altrep_info altrep_info = {};
    altrep_info.length = XLENGTH(x);
    altrep_info.private_data = x;
    altrep_info.type = TYPEOF(x);
    altrep_info.operations.get_region = read_altrep;
    SEXP altptr_object = Travel_make_altptr(altrep_info);
    return altptr_object;
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
    int max_print = (XLENGTH(x)>100?100:XLENGTH(x));
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
SEXP C_get_ptr(SEXP x){
    return R_MakeExternalPtr(DATAPTR(x),R_NilValue,R_NilValue);
}




