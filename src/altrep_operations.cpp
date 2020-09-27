//#include <sys/types.h>
//Include Rcpp header in utils.h
#include <string>
#include "Rcpp.h"
#include "R_ext/Altrep.h"
#include "package_settings.h"
#include "filesystem_manager.h"
#include "Travel.h"
#include "memory_mapped_file.h"
#define UTILS_ENABLE_R
#include "utils.h"
#undef UTILS_ENABLE_R


#define SLOT_NUM 4
#define NAME_SLOT 0
#define FILE_HANDLE_SLOT 1
#define SIZE_SLOT 2
#define LENGTH_SLOT 3

#define GET_WRAPPED_DATA(x) R_altrep_data1(x)
#define SET_WRAPPED_DATA(x, v) R_set_altrep_data1(x, v)

#define GET_PROPS(x) ((Rcpp::List)R_altrep_data2(x))

//These macros assume x is a list of options
#define GET_NAME(x) (x[NAME_SLOT])
//File handle external pointer
#define GET_HANDLE_EXTPTR(x) (x[FILE_HANDLE_SLOT])
//File handle itself
#define GET_FILE_HANDLE(x) ((file_map_handle *)R_ExternalPtrAddr(GET_HANDLE_EXTPTR(x)))
#define GET_SIZE(x) (x[SIZE_SLOT])
#define GET_LENGTH(x) (x[LENGTH_SLOT])

//These macros assume x is an ALTREP object
#define GET_ALT_NAME(x) (GET_NAME(GET_PROPS(x)))
#define GET_ALT_HANDLE_EXTPTR(x) (GET_HANDLE_EXTPTR(GET_PROPS(x)))
#define GET_ALT_FILE_HANDLE(x) (GET_FILE_HANDLE(GET_PROPS(x)))
#define GET_ALT_SIZE(x) (GET_SIZE(GET_PROPS(x)))
#define GET_ALT_LENGTH(x) (GET_LENGTH(GET_PROPS(x)))


R_altrep_class_t altPtr_real_class;
R_altrep_class_t altPtr_integer_class;
R_altrep_class_t altPtr_logical_class;
//R_altrep_class_t altPtr_raw_class;
//R_altrep_class_t altPtr_complex_class;

R_altrep_class_t getAltClass(int type)
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
        //return altPtr_raw_class;
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

static void file_handle_finalizer(SEXP handle_extptr)
{
    Rcpp::List altptr_options = R_ExternalPtrTag(handle_extptr);
    std::string name = Rcpp::as<std::string>(GET_NAME(altptr_options));
    file_map_handle *handle = (file_map_handle *)R_ExternalPtrAddr(handle_extptr);
    if (!has_mapped_file_handle(handle))
    {
        Rf_warning("The altptr file handle has been released: %s, handle: %p\n", name.c_str(), handle);
    }
    else
    {
        debug_print("Finalizer, name:%s, size:%llu\n", name.c_str(), handle->size);
        std::string status = memory_unmap(handle);
        if (status != "")
        {
            Rf_warning(status.c_str());
        }
    }
    remove_virtual_file(name);
}

SEXP Travel_make_altrep(int type, size_t length, file_data_func read_func, void *data, unsigned int unit_size, SEXP protect)
{
    if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    PROTECT_GUARD guard;
    R_altrep_class_t alt_class = getAltClass(type);
    Rcpp::List altptr_options(SLOT_NUM);
    GET_LENGTH(altptr_options) = length;
    SEXP result = guard.protect(R_new_altrep(alt_class, protect, altptr_options));
    //Compute the total size
    size_t size = length * unit_size;
    GET_SIZE(altptr_options) = size;
    //Create a virtual file
    filesystem_file_info file_info = add_virtual_file(read_func, data, size, unit_size);
    std::string file_name = file_info.file_name;
    GET_NAME(altptr_options) = file_name;
    file_map_handle *handle;
    std::string status = memory_map(handle, file_info, size);
    if (status != "")
    {
        remove_virtual_file(file_info.file_name);
        Rf_warning(status.c_str());
        return R_NilValue;
    }
    SEXP handle_extptr = guard.protect(R_MakeExternalPtr(handle, altptr_options, R_NilValue));
    //Register finalizer for the pointer
    R_RegisterCFinalizerEx(handle_extptr, file_handle_finalizer, TRUE);
    GET_HANDLE_EXTPTR(altptr_options) = handle_extptr;
    return result;
}

SEXP get_file_name(SEXP x){
    return GET_ALT_NAME(x);
}

void flush_altptr(SEXP x){
    file_map_handle *handle = GET_ALT_FILE_HANDLE(x);
    std::string status = flush_handle(handle);
    if(status!=""){
        Rf_warning(status.c_str());
    }
}

// [[Rcpp::export]]
void C_flush_altptr(SEXP x){
    flush_altptr(x);
}

/*
==========================================
ALTREP operations
==========================================
*/
Rboolean altPtr_Inspect(SEXP x, int pre, int deep, int pvec,
                        void (*inspect_subtree)(SEXP, int, int, int))
{
    Rprintf("altrep pointer object(len=%llu, type=%d)\n", Rf_xlength(x), TYPEOF(x));
    return TRUE;
}

R_xlen_t altPtr_length(SEXP x)
{
    R_xlen_t size = Rcpp::as<size_t>(GET_ALT_LENGTH(x));
    altrep_print("accessing length: %d\n", size);
    return size;
}

void *altPtr_dataptr(SEXP x, Rboolean writeable)
{
    altrep_print("accessing data pointer\n");
    if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    SEXP handle_extptr = GET_ALT_HANDLE_EXTPTR(x);
    if (handle_extptr == R_NilValue)
    {
        return NULL;
    }
    else
    {
        file_map_handle *handle = (file_map_handle *)R_ExternalPtrAddr(handle_extptr);
        if (has_mapped_file_handle(handle))
        {
            return handle->ptr;
        }
        else
        {
            Rf_error("The file handle has been released!\n");
        }
    }
}
const void *altPtr_dataptr_or_null(SEXP x)
{
    altrep_print("accessing data pointer or null\n");
    if (is_filesystem_running())
    {
        return altPtr_dataptr(x, Rboolean::TRUE);
    }
    else
    {
        return NULL;
    }
}


/*
Register ALTREP class
*/

#define ALT_COMMOM_REGISTRATION(ALT_CLASS, ALT_TYPE, FUNC_PREFIX)         \
    ALT_CLASS = R_make_##ALT_TYPE##_class(class_name, PACKAGE_NAME, dll); \
    /* common ALTREP methods */                                           \
    R_set_altrep_Inspect_method(ALT_CLASS, altPtr_Inspect);               \
    R_set_altrep_Length_method(ALT_CLASS, altPtr_length);                 \
    /* ALTVEC methods */                                                  \
    R_set_altvec_Dataptr_method(ALT_CLASS, altPtr_dataptr);               \
    R_set_altvec_Dataptr_or_null_method(ALT_CLASS, altPtr_dataptr_or_null);

//[[Rcpp::init]]
void init_logical_class(DllInfo *dll)
{
    char class_name[] = "altPtr_logical";
    ALT_COMMOM_REGISTRATION(altPtr_logical_class, altlogical, LOGICAL)
}
//[[Rcpp::init]]
void init_integer_class(DllInfo *dll)
{
    char class_name[] = "altPtr_integer";
    ALT_COMMOM_REGISTRATION(altPtr_integer_class, altinteger, INTEGER)
}

//[[Rcpp::init]]
void init_real_class(DllInfo *dll)
{
    char class_name[] = "shared_real";
    ALT_COMMOM_REGISTRATION(altPtr_real_class, altreal, REAL)
}


