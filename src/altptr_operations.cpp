#include "altrep.h"
#include "utils.h"
#include "filesystem_manager.h"
#include "memory_mapped_file.h"
#include "package_settings.h"
#include "Travel.h"

R_altrep_class_t altPtr_real_class;
R_altrep_class_t altPtr_integer_class;
R_altrep_class_t altPtr_logical_class;
R_altrep_class_t altPtr_raw_class;
//R_altrep_class_t altPtr_complex_class;

R_altrep_class_t get_altptr_class(int type)
{
    switch (type)
    {
    case REALSXP:
        return altPtr_real_class;
    case INTSXP:
        return altPtr_integer_class;
    case LGLSXP:
        return altPtr_logical_class;
    case RAWSXP:
        return altPtr_raw_class;
    case CPLXSXP:
        //return altPtr_complex_class;
    case STRSXP:
        //return altPtr_str_class;
    default:
        Rf_error("Type of %d is not supported yet", type);
    }
    // Just for suppressing the annoying warning, it should never be excuted
    return altPtr_real_class;
}
/*
==========================================
ALTREP operations
==========================================
*/

Rboolean altptr_Inspect(SEXP x, int pre, int deep, int pvec,
                        void (*inspect_subtree)(SEXP, int, int, int))
{
    Rprintf("altrep pointer object(len=%llu, type=%d)\n", Rf_xlength(x), TYPEOF(x));
    return TRUE;
}

R_xlen_t altptr_length(SEXP x)
{
    R_xlen_t size = Rcpp::as<size_t>(GET_ALT_LENGTH(x));
    altrep_print("accessing length: %d\n", size);
    return size;
}

void *altptr_dataptr(SEXP x, Rboolean writeable)
{
    altrep_print("accessing data pointer\n");
    if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    SEXP handle_extptr = GET_ALT_HANDLE_EXTPTR(x);
    if (handle_extptr == R_NilValue)
    {
        Rf_error("The file handle is NULL, this should not happen!\n");
    }
    file_map_handle *handle = (file_map_handle *)R_ExternalPtrAddr(handle_extptr);
    if (!has_mapped_file_handle(handle))
    {
        Rf_error("The file handle has been released!\n");
    }
    return handle->ptr;
}

const void *altptr_dataptr_or_null(SEXP x)
{
    altrep_print("accessing data pointer or null\n");
    if (is_filesystem_running())
    {
        return altptr_dataptr(x, Rboolean::TRUE);
    }
    else
    {
        return NULL;
    }
}


SEXP altptr_duplicate(SEXP x, Rboolean deep) {
	if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    SEXP handle_extptr = GET_ALT_HANDLE_EXTPTR(x);
    file_map_handle* handle = (file_map_handle *)R_ExternalPtrAddr(handle_extptr);
    //Pass all changes to the filesystem
    flush_handle(handle);
    //Get the file name
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    filesystem_file_data& file_data = get_virtual_file(file_name);
    //Duplicate the object
    SEXP res = PROTECT(Travel_make_altptr(TYPEOF(x), XLENGTH(x), file_data.data_func, file_data.private_data, GET_WRAPPED_DATA(x)));
    //Get the new file name
    std::string new_file_name = Rcpp::as<std::string>(GET_ALT_NAME(res));
    filesystem_file_data& new_file_data = get_virtual_file(new_file_name);
    claim(file_data.cache_size==new_file_data.cache_size);
    size_t cache_size = file_data.cache_size;
    for(auto i:file_data.write_cache){
        char* ptr = new char[cache_size];
        memcpy(ptr, i.second, cache_size);
        new_file_data.write_cache[i.first] = ptr;
    }
    Rf_unprotect(1);
    return res;
}

/*
Register ALTREP class
*/

#define ALT_COMMOM_REGISTRATION(ALT_CLASS, ALT_TYPE, FUNC_PREFIX)         \
    ALT_CLASS = R_make_##ALT_TYPE##_class(class_name, PACKAGE_NAME, dll); \
    /* common ALTREP methods */                                           \
    R_set_altrep_Inspect_method(ALT_CLASS, altptr_Inspect);               \
    R_set_altrep_Length_method(ALT_CLASS, altptr_length);                 \
    R_set_altrep_Duplicate_method(ALT_CLASS, altptr_duplicate);     \
    /* ALTVEC methods */                                                  \
    R_set_altvec_Dataptr_method(ALT_CLASS, altptr_dataptr);               \
    R_set_altvec_Dataptr_or_null_method(ALT_CLASS, altptr_dataptr_or_null);
	
    
    //R_set_altvec_Extract_subset_method(ALT_CLASS, numeric_subset<R_TYPE, C_TYPE>);\


//[[Rcpp::init]]
void init_altptr_logical_class(DllInfo *dll)
{
    char class_name[] = "travel_altptr_logical";
    ALT_COMMOM_REGISTRATION(altPtr_logical_class, altlogical, LOGICAL)
}
//[[Rcpp::init]]
void init_altptr_integer_class(DllInfo *dll)
{
    char class_name[] = "travel_altptr_integer";
    ALT_COMMOM_REGISTRATION(altPtr_integer_class, altinteger, INTEGER)
}

//[[Rcpp::init]]
void ini_altptrt_real_class(DllInfo *dll)
{
    char class_name[] = "travel_altptr_real";
    ALT_COMMOM_REGISTRATION(altPtr_real_class, altreal, REAL)
}

//[[Rcpp::init]]
void init_altptr_raw_class(DllInfo *dll)
{
    char class_name[] = "travel_altptr_raw";
    ALT_COMMOM_REGISTRATION(altPtr_real_class, altraw, RAW)
}
