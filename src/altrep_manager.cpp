#include <string>
#include "altrep.h"
#include "package_settings.h"
#include "filesystem_manager.h"
#include "Travel.h"
#include "memory_mapped_file.h"
#define UTILS_ENABLE_R
#include "utils.h"
#undef UTILS_ENABLE_R

static void altptr_handle_finalizer(SEXP handle_extptr)
{
    std::string name = Rcpp::as<std::string>(R_ExternalPtrTag(handle_extptr));
    file_map_handle *handle = (file_map_handle *)R_ExternalPtrAddr(handle_extptr);
    if (!has_mapped_file_handle(handle))
    {
        Rf_warning("The altptr handle has been released: %s, handle: %p\n", name.c_str(), handle);
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
SEXP Travel_make_altptr_internal(Filesystem_file_info& file_info)
{
    Filesystem_file_data &file_data = get_filesystem_file_data(file_info.file_name);
    if(file_data.altrep_info.protected_data==NULL){
        file_data.altrep_info.protected_data = R_NilValue;
    }
    if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    PROTECT_GUARD guard;
    R_altrep_class_t alt_class = get_altptr_class(file_data.coerced_type);
    SEXP altptr_options = guard.protect(Rf_allocVector(VECSXP, SLOT_NUM));
    SET_PROPS_LENGTH(altptr_options, Rcpp::wrap(file_data.altrep_info.length));
    SEXP result = guard.protect(R_new_altrep(alt_class, file_data.altrep_info.protected_data, altptr_options));
    
    SET_PROPS_NAME(altptr_options, Rcpp::wrap(file_info.file_name));
    SET_PROPS_SIZE(altptr_options, Rcpp::wrap(file_data.file_size));
    file_map_handle *handle;
    std::string status = memory_map(handle, file_info, file_data.file_size);
    if (status != "")
    {
        remove_filesystem_file(file_info.file_name);
        Rf_warning(status.c_str());
        return R_NilValue;
    }
    SEXP handle_extptr = guard.protect(R_MakeExternalPtr(handle,
                                                         GET_PROPS_NAME(altptr_options),
                                                         R_NilValue));
    //Register finalizer for the pointer
    R_RegisterCFinalizerEx(handle_extptr, altptr_handle_finalizer, TRUE);
    SET_PROPS_EXTPTR(altptr_options, handle_extptr);
    return result;
}



SEXP Travel_make_altptr(Travel_altrep_info altrep_info){
    Subset_index index(altrep_info.length);
    //Create a virtual file
    Filesystem_file_info file_info = add_filesystem_file(altrep_info.type,index, altrep_info);
    return Travel_make_altptr_internal(file_info);
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
        std::string status = memory_unmap(handle, false);
        if (status != "")
        {
            Rf_warning(status.c_str());
        }
    }
}

SEXP make_altptr_from_file(std::string path, int type, size_t length)
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
    Filesystem_file_info file_info;
    file_info.file_name = file_name;
    file_info.file_full_path = path;
    file_map_handle *handle;
    std::string status = memory_map(handle, file_info, size, false);
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
    return Rf_mkString(handle->file_info.file_full_path.c_str());
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
SEXP C_get_altptr_cache(SEXP x)
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
void C_print_cache(SEXP x, size_t i){
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    if(file_data.write_cache.find(i)!=file_data.write_cache.end()){
        const double* ptr = (const double *) file_data.write_cache.find(i)->second.get_const();
        for(size_t j = 0; j<4096/8;j++){
            Rprintf("%f,", ptr[j]);
        }
    }
}
