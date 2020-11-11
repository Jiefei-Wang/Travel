#include <Rcpp.h>
#include <R_ext/Altrep.h>
#include "altrep_macros.h"
#include "memory_mapped_file.h"
#include "package_settings.h"
#include "Travel.h"
#define UTILS_ENABLE_R
#include "utils.h"


R_altrep_class_t altfile_real_class;
R_altrep_class_t altfile_integer_class;
R_altrep_class_t altfile_logical_class;
R_altrep_class_t altfile_raw_class;

R_altrep_class_t get_altfile_class(int type)
{
    switch (type)
    {
    case REALSXP:
        return altfile_real_class;
    case INTSXP:
        return altfile_integer_class;
    case LGLSXP:
        return altfile_logical_class;
    case RAWSXP:
        return altfile_raw_class;
    default:
        Rf_error("Type of %d is not supported yet", type);
    }
    // Just for suppressing the annoying warning, it should never be excuted
    return altfile_real_class;
}
/*
==========================================
ALTREP operations
==========================================
*/

Rboolean altfile_Inspect(SEXP x, int pre, int deep, int pvec,
                        void (*inspect_subtree)(SEXP, int, int, int))
{
    Rprintf("altrep file object(len=%llu, type=%d)\n", Rf_xlength(x), TYPEOF(x));
    return TRUE;
}

R_xlen_t altfile_length(SEXP x)
{
    R_xlen_t size = Rcpp::as<size_t>(GET_ALT_LENGTH(x));
    altrep_print("accessing length: %llu\n", size);
    return size;
}

void *altfile_dataptr(SEXP x, Rboolean writeable)
{
    altrep_print("accessing data pointer\n");
    SEXP handle_extptr = GET_ALT_HANDLE_EXTPTR(x);
    if (handle_extptr == R_NilValue)
    {
        Rf_error("The file handle is NULL, this should not happen!\n");
    }
    file_map_handle *handle = (file_map_handle *)R_ExternalPtrAddr(handle_extptr);
    return handle->ptr;
}

const void *altfile_dataptr_or_null(SEXP x)
{
    altrep_print("accessing data pointer or null\n");
    return altfile_dataptr(x, Rboolean::TRUE);
}



/*
Register ALTREP class
*/

#define ALT_COMMOM_REGISTRATION(ALT_CLASS, ALT_TYPE, FUNC_PREFIX)         \
    ALT_CLASS = R_make_##ALT_TYPE##_class(class_name, PACKAGE_NAME, dll); \
    /* common ALTREP methods */                                           \
    R_set_altrep_Inspect_method(ALT_CLASS, altfile_Inspect);               \
    R_set_altrep_Length_method(ALT_CLASS, altfile_length);                 \
    /* ALTVEC methods */                                                  \
    R_set_altvec_Dataptr_method(ALT_CLASS, altfile_dataptr);               \
    R_set_altvec_Dataptr_or_null_method(ALT_CLASS, altfile_dataptr_or_null);
	
    
//R_set_altvec_Extract_subset_method(ALT_CLASS, numeric_subset<R_TYPE, C_TYPE>);


//[[Rcpp::init]]
void init_altfile_logical_class(DllInfo *dll)
{
    char class_name[] = "travel_altfile_logical";
    ALT_COMMOM_REGISTRATION(altfile_logical_class, altlogical, LOGICAL);
}
//[[Rcpp::init]]
void init_altfile_integer_class(DllInfo *dll)
{
    char class_name[] = "travel_altfile_integer";
    ALT_COMMOM_REGISTRATION(altfile_integer_class, altinteger, INTEGER);
}

//[[Rcpp::init]]
void ini_altfile_real_class(DllInfo *dll)
{
    char class_name[] = "travel_altfile_real";
    ALT_COMMOM_REGISTRATION(altfile_real_class, altreal, REAL);
}

//[[Rcpp::init]]
void init_altfile_raw_class(DllInfo *dll)
{
    char class_name[] = "travel_altfile_raw";
    ALT_COMMOM_REGISTRATION(altfile_raw_class, altraw, RAW);
}



