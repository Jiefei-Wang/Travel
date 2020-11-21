#include <string>
#include <Rcpp.h>
#include <R_ext/Altrep.h>
#include "altmmap_operations.h"
#include "altrep_macros.h"
//#include "package_settings.h"
#include "filesystem_manager.h"
#include "memory_mapped_file.h"
#define UTILS_ENABLE_R
#include "utils.h"
#undef UTILS_ENABLE_R

static void altmmap_handle_finalizer(SEXP handle_extptr)
{
    std::string name = Rcpp::as<std::string>(R_ExternalPtrTag(handle_extptr));
    file_map_handle *handle = (file_map_handle *)R_ExternalPtrAddr(handle_extptr);
    if (!has_mapped_file_handle(handle))
    {
        Rf_warning("The altmmap handle has been released: %s, handle: %p\n", name.c_str(), handle);
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
    remove_filesystem_file(name);
}

/*
The internal function allows to create an altrep object with specific type
that is not consistent with the type in the altrep_info
*/
SEXP Travel_make_altmmap(Filesystem_file_identifier &file_info)
{
    Filesystem_file_data &file_data = get_filesystem_file_data(file_info.file_name);
    if (file_data.altrep_info.protected_data == NULL)
    {
        file_data.altrep_info.protected_data = R_NilValue;
    }
    if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    PROTECT_GUARD guard;
    R_altrep_class_t alt_class = get_altmmap_class(file_data.coerced_type);
    SEXP altmmap_options = guard.protect(Rf_allocVector(VECSXP, SLOT_NUM));
    SET_PROPS_LENGTH(altmmap_options, Rcpp::wrap(file_data.file_length));
    SEXP result = guard.protect(R_new_altrep(alt_class, file_data.altrep_info.protected_data, altmmap_options));

    SET_PROPS_NAME(altmmap_options, Rcpp::wrap(file_info.file_name));
    SET_PROPS_SIZE(altmmap_options, Rcpp::wrap(file_data.file_size));
    file_map_handle *handle;
    std::string status = memory_map(handle, file_info.file_full_path, file_data.file_size);
    if (status != "")
    {
        remove_filesystem_file(file_info.file_name);
        Rf_warning(status.c_str());
        return R_NilValue;
    }
    SEXP handle_extptr = guard.protect(R_MakeExternalPtr(handle,
                                                         GET_PROPS_NAME(altmmap_options),
                                                         R_NilValue));
    //Register finalizer for the pointer
    R_RegisterCFinalizerEx(handle_extptr, altmmap_handle_finalizer, TRUE);
    SET_PROPS_EXTPTR(altmmap_options, handle_extptr);
    return result;
}

SEXP Travel_make_altmmap(Travel_altrep_info &altrep_info)
{
    Subset_index index(altrep_info.length);
    //Create a virtual file
    Filesystem_file_identifier file_info = add_filesystem_file(altrep_info.type, index, altrep_info);
    return Travel_make_altmmap(file_info);
}
SEXP Travel_make_altrep(Travel_altrep_info altrep_info)
{
    return Travel_make_altmmap(altrep_info);
}

static void altfile_handle_finalizer(SEXP handle_extptr)
{
    std::string name = Rcpp::as<std::string>(R_ExternalPtrTag(handle_extptr));
    file_map_handle *handle = (file_map_handle *)R_ExternalPtrAddr(handle_extptr);
    if (!has_mapped_file_handle(handle))
    {
        Rf_warning("The altfile handle has been released: %s, handle: %p\n", name.c_str(), handle);
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
}

R_altrep_class_t get_altfile_class(int type);
SEXP make_altmmap_from_file(std::string path, int type, size_t length)
{
    PROTECT_GUARD guard;
    R_altrep_class_t alt_class = get_altfile_class(type);
    SEXP altfile_options = guard.protect(Rf_allocVector(VECSXP, SLOT_NUM));
    SET_PROPS_LENGTH(altfile_options, Rcpp::wrap(length));
    SEXP result = guard.protect(R_new_altrep(alt_class, R_NilValue, altfile_options));
    //Compute the total size
    size_t unit_size = get_type_size(type);
    size_t size = length * unit_size;
    SET_PROPS_SIZE(altfile_options, Rcpp::wrap(size));
    std::string file_name = get_file_name_in_path(path);
    SET_PROPS_NAME(altfile_options, Rcpp::wrap(file_name));
    Filesystem_file_identifier file_info;
    file_info.file_name = file_name;
    file_info.file_full_path = path;
    file_map_handle *handle;
    std::string status = memory_map(handle, file_info.file_full_path, size, false);
    if (status != "")
    {
        Rf_warning(status.c_str());
        return R_NilValue;
    }
    SEXP handle_extptr = guard.protect(R_MakeExternalPtr(handle,
                                                         GET_PROPS_NAME(altfile_options),
                                                         R_NilValue));
    //Register finalizer for the pointer
    R_RegisterCFinalizerEx(handle_extptr, altfile_handle_finalizer, TRUE);
    SET_PROPS_EXTPTR(altfile_options, handle_extptr);
    return result;
}

SEXP get_file_name(SEXP x)
{
    return GET_ALT_NAME(x);
}
SEXP get_file_path(SEXP x)
{
    file_map_handle *handle = GET_ALT_FILE_HANDLE(x);
    return Rcpp::wrap(handle->file_path);
}

void flush_altrep(SEXP x)
{
    file_map_handle *handle = GET_ALT_FILE_HANDLE(x);
    std::string status = flush_handle(handle);
    if (status != "")
    {
        Rf_warning(status.c_str());
    }
}
