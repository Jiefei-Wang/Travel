//#include <sys/types.h>
//Include Rcpp header in utils.h
#define UTILS_ENABLE_R
#include <string>
#include "Rcpp.h"
#include "R_ext/Altrep.h"
#include "utils.h"
#include "package_settings.h"
#include "filesystem_manager.h"
#include "read_operations.h"
#include "altrep_operations.h"
#include "memory_mapped_file.h"

#define SLOT_NUM 4
#define NAME_SLOT 0
#define PTR_SLOT 1
#define SIZE_SLOT 2
#define LENGTH_SLOT 3

#define GET_WRAPPED_DATA(x) R_altrep_data1(x)
#define SET_WRAPPED_DATA(x, v) R_set_altrep_data1(x, v)

#define GET_PROPS(x) ((Rcpp::List)R_altrep_data2(x))

#define GET_NAME(x) (x[NAME_SLOT])
#define GET_PTR(x) (x[PTR_SLOT])
#define GET_SIZE(x) (x[SIZE_SLOT])
#define GET_LENGTH(x) (x[LENGTH_SLOT])

#define GET_ALT_NAME(x) (GET_PROPS(x)[NAME_SLOT])
#define GET_ALT_PTR(x) (GET_PROPS(x)[PTR_SLOT])
#define GET_ALT_SIZE(x) (GET_PROPS(x)[SIZE_SLOT])
#define GET_ALT_LENGTH(x) (GET_PROPS(x)[LENGTH_SLOT])

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
    SEXP handle_extptr = GET_ALT_PTR(x);
    if (handle_extptr == R_NilValue)
    {
        return NULL;
    }
    else
    {
        return ((file_map_handle*)R_ExternalPtrAddr(handle_extptr))->ptr;
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

static void ptr_finalizer(SEXP handle_extptr)
{
    Rcpp::List altptr_options = R_ExternalPtrTag(handle_extptr);
    std::string name = Rcpp::as<std::string>(GET_NAME(altptr_options));
    file_map_handle* handle= (file_map_handle*)R_ExternalPtrAddr(handle_extptr);
    if(!has_mapped_file_handle(handle)){
        Rf_warning("The altptr file handle has been released: %s, handle: %p\n", name.c_str(),handle);
        return;
    }
    debug_print("Finalizer, name:%s, size:%llu\n", name.c_str(), handle->size);
    std::string status = memory_unmap(handle);
    if (status != "")
    {
        Rf_warning(status.c_str());
    }
    remove_virtual_file(name);
}

SEXP make_altptr(int type, void *data, size_t length, unsigned int unit_size, file_data_func read_func, SEXP protect)
{
    if(!is_filesystem_running()){
        Rf_error("The filesystem is not running!\n");
    }
    PROTECT_GUARD guard;
    R_altrep_class_t alt_class = getAltClass(type);
    Rcpp::List altptr_options(SLOT_NUM);
    GET_LENGTH(altptr_options) = length;
    SEXP result = guard.protect(R_new_altrep(alt_class, protect, altptr_options));
    //Compute the total size
    size_t size = length*unit_size;
    GET_SIZE(altptr_options) = size;
    //Fill the file data that is required by the filesystem
    filesystem_file_data file_data(read_func,data,size,unit_size);
    filesystem_file_info file_info = add_virtual_file(file_data);
    std::string file_name = file_info.file_name;
    GET_NAME(altptr_options) = file_name;
    file_map_handle *handle;
    std::string status = memory_map(handle, file_info, size);
    if (status != "")
    {
        Rf_warning(status.c_str());
        return R_NilValue;
    }
    SEXP handle_extptr = guard.protect(R_MakeExternalPtr(handle, altptr_options, R_NilValue));
    //Register finalizer for the pointer
    R_RegisterCFinalizerEx(handle_extptr, ptr_finalizer, TRUE);
    GET_PTR(altptr_options) = handle_extptr;
    return result;
}



