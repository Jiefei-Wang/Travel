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
    Memory_mapped *handle = (Memory_mapped *)R_ExternalPtrAddr(handle_extptr);
    if (!handle->is_mapped())
    {
        Rprintf("The altmmap handle has been released: %s, handle: %p\n", name.c_str(), handle);
    }
    else
    {
        debug_print("Finalizer, name:%s, size:%llu\n", name.c_str(), handle->get_size());
        bool status = handle->unmap();
        if (!status)
        {
            Rprintf(handle->get_last_error().c_str());
        }
    }
    unregister_file_handle(handle);
    delete handle;
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
    Memory_mapped *handle = new Memory_mapped(file_info.file_full_path,file_data.file_size);
    if (!handle->is_mapped())
    {
        remove_filesystem_file(file_info.file_name);
        Rf_warning(handle->get_last_error().c_str());
        delete handle;
        return R_NilValue;
    }
    register_file_handle(handle);
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
    Subset_index index(0,altrep_info.length);
    //Create a virtual file
    Filesystem_file_identifier file_info = add_filesystem_file(altrep_info.type, index, altrep_info);
    return Travel_make_altmmap(file_info);
}

static void altfile_handle_finalizer(SEXP handle_extptr)
{
    std::string name = Rcpp::as<std::string>(R_ExternalPtrTag(handle_extptr));
    Memory_mapped *handle = (Memory_mapped *)R_ExternalPtrAddr(handle_extptr);
    if (!handle->is_mapped())
    {
        Rprintf("The altfile handle has been released: %s, handle: %p\n", name.c_str(), handle);
    }
    else
    {
        Rprintf("Altfile finalizer, name:%s, size:%llu\n", name.c_str(), handle->get_size());
        bool status = handle->unmap();
        if (!status)
        {
            Rf_warning(handle->get_last_error().c_str());
        }
    }
    delete handle;
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
    Memory_mapped *handle= new Memory_mapped(file_info.file_full_path, size);
    if (!handle->is_mapped())
    {
        Rf_warning(handle->get_last_error().c_str());
        delete handle;
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
    Memory_mapped *handle = GET_ALT_FILE_HANDLE(x);
    return Rcpp::wrap(handle->get_file_path());
}

void flush_altrep(SEXP x)
{
    Memory_mapped *handle = GET_ALT_FILE_HANDLE(x);
    bool status = handle->flush();
    if (!status)
    {
        Rf_warning(handle->get_last_error().c_str());
    }
}
