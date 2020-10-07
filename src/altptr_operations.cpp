#include "altrep.h"
#include "utils.h"
#include "filesystem_manager.h"
#include "memory_mapped_file.h"
#include "package_settings.h"
#include "Travel.h"
#include "read_write_operations.h"

R_altrep_class_t altptr_real_class;
R_altrep_class_t altptr_integer_class;
R_altrep_class_t altptr_logical_class;
R_altrep_class_t altptr_raw_class;
//R_altrep_class_t altptr_complex_class;

R_altrep_class_t get_altptr_class(int type)
{
    switch (type)
    {
    case REALSXP:
        return altptr_real_class;
    case INTSXP:
        return altptr_integer_class;
    case LGLSXP:
        return altptr_logical_class;
    case RAWSXP:
        return altptr_raw_class;
    case CPLXSXP:
        //return altptr_complex_class;
    case STRSXP:
        //return altptr_str_class;
    default:
        Rf_error("Type of %d is not supported yet", type);
    }
    // Just for suppressing the annoying warning, it should never be excuted
    return altptr_real_class;
}
/*
==========================================
ALTREP operations
==========================================
*/

Rboolean altptr_Inspect(SEXP x, int pre, int deep, int pvec,
                        void (*inspect_subtree)(SEXP, int, int, int))
{
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_virtual_file(file_name);
    Rprintf("Altptr: data type: %s, len: %llu, size: %llu, cache num: %llu, private: %p",
            get_type_name(file_data.altrep_info.type).c_str(),
            (uint64_t)Rf_xlength(x),
            (uint64_t)file_data.file_size,
            (uint64_t)file_data.write_cache.size(),
            file_data.altrep_info.private_data);
    if (file_data.altrep_info.operations.get_private_size != NULL)
    {
        size_t private_size = file_data.altrep_info.operations.get_private_size(&file_data.altrep_info);
        Rprintf(", private size:%llu\n", (uint64_t)private_size);
    }
    else
    {
        Rprintf("\n");
    }
    if (file_data.altrep_info.private_data != NULL &&
        file_data.altrep_info.operations.inspect_private != NULL)
    {
        Rprintf("[Private data]\n");
        file_data.altrep_info.operations.inspect_private(&file_data.altrep_info);
    }
    if (file_data.write_cache.size() > 0)
    {
        Rprintf("[Cache info]\n");
        for (const auto &i : file_data.write_cache)
        {
            Rprintf("\tCache block %llu, shared number %llu, ptr: %p\n", i.first, i.second.use_count(), i.second.get_const());
        }
    }
    return TRUE;
}

R_xlen_t altptr_length(SEXP x)
{
    R_xlen_t size = Rcpp::as<size_t>(GET_ALT_LENGTH(x));
    altrep_print("accessing length: %llu\n", size);
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

SEXP altptr_duplicate(SEXP x, Rboolean deep)
{
    altrep_print("Duplicating object\n");
    if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    //Pass all changes to the filesystem
    flush_altrep(x);
    //Get the file name
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &old_file_data = get_virtual_file(file_name);
    //Duplicate the object
    SEXP res = PROTECT(Travel_make_altptr(old_file_data.altrep_info));
    //Get the new file name
    std::string new_file_name = Rcpp::as<std::string>(GET_ALT_NAME(res));
    Filesystem_file_data &new_file_data = get_virtual_file(new_file_name);
    new_file_data.coerced_type = old_file_data.coerced_type;
    //Copy write cache
    claim(old_file_data.cache_size == new_file_data.cache_size);
    new_file_data.write_cache = old_file_data.write_cache;
    Rf_unprotect(1);
    return res;
}

SEXP altptr_coerce(SEXP x, int type)
{
    altrep_print("Coercing object\n");
    if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    //Pass all changes to the filesystem
    flush_altrep(x);
    //Get the file name
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &old_file_data = get_virtual_file(file_name);

    //Create the new altrep info
    Travel_altrep_info new_altrep_info;
    if (old_file_data.altrep_info.operations.coerce == NULL)
        new_altrep_info = old_file_data.altrep_info;
    else
        new_altrep_info = old_file_data.altrep_info.operations.coerce(&old_file_data.altrep_info, type);

    //Duplicate the object
    SEXP res = PROTECT(Travel_make_altptr_internal(type, new_altrep_info));
    std::string new_file_name = Rcpp::as<std::string>(GET_ALT_NAME(res));
    Filesystem_file_data &new_file_data = get_virtual_file(new_file_name);

    //Convert the write_cache
    size_t old_file_unit_size = get_type_size(old_file_data.coerced_type);
    size_t new_file_unit_size = get_type_size(new_file_data.coerced_type);
    size_t buffer_size = std::min(
        old_file_data.cache_size,
        old_file_data.cache_size / old_file_unit_size * new_file_unit_size);
    std::unique_ptr<char[]> buffer(new char[buffer_size]);
    for (const auto &i : old_file_data.write_cache)
    {
        size_t read_offset = i.first * old_file_data.cache_size;
        size_t read_size = get_valid_file_size(old_file_data.file_size, read_offset, old_file_data.cache_size);
        general_read_func(old_file_data, buffer.get(), read_offset, read_size);
        copy_memory(new_file_data.coerced_type, old_file_data.coerced_type,
                    buffer.get(), buffer.get(),
                    old_file_data.cache_size / old_file_unit_size,
                    new_file_unit_size > old_file_unit_size);
        size_t write_offset = read_offset / old_file_unit_size * new_file_unit_size;
        size_t write_size = read_size / old_file_unit_size * new_file_unit_size;
        general_write_func(new_file_data, buffer.get(), write_offset, write_size);
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
    R_set_altrep_Duplicate_method(ALT_CLASS, altptr_duplicate);           \
    R_set_altrep_Coerce_method(ALT_CLASS, altptr_coerce);                 \
    /* ALTVEC methods */                                                  \
    R_set_altvec_Dataptr_method(ALT_CLASS, altptr_dataptr);               \
    R_set_altvec_Dataptr_or_null_method(ALT_CLASS, altptr_dataptr_or_null);

//R_set_altvec_Extract_subset_method(ALT_CLASS, numeric_subset<R_TYPE, C_TYPE>);

//[[Rcpp::init]]
void init_altptr_logical_class(DllInfo *dll)
{
    char class_name[] = "travel_altptr_logical";
    ALT_COMMOM_REGISTRATION(altptr_logical_class, altlogical, LOGICAL);
}
//[[Rcpp::init]]
void init_altptr_integer_class(DllInfo *dll)
{
    char class_name[] = "travel_altptr_integer";
    ALT_COMMOM_REGISTRATION(altptr_integer_class, altinteger, INTEGER);
}

//[[Rcpp::init]]
void ini_altptr_real_class(DllInfo *dll)
{
    char class_name[] = "travel_altptr_real";
    ALT_COMMOM_REGISTRATION(altptr_real_class, altreal, REAL);
}

//[[Rcpp::init]]
void init_altptr_raw_class(DllInfo *dll)
{
    char class_name[] = "travel_altptr_raw";
    ALT_COMMOM_REGISTRATION(altptr_raw_class, altraw, RAW);
}
