#include "altrep.h"
#define UTILS_ENABLE_R
#include "utils.h"
#include "filesystem_manager.h"
#include "memory_mapped_file.h"
#include "package_settings.h"
#include "Travel.h"
#include "class_Filesystem_cache_copier.h"

size_t default_subset_length_cutoff = 3;

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

bool is_altptr(SEXP x)
{
    if (R_altrep_inherits(x, altptr_real_class))
    {
        return true;
    }
    if (R_altrep_inherits(x, altptr_integer_class))
    {
        return true;
    }
    if (R_altrep_inherits(x, altptr_logical_class))
    {
        return true;
    }
    if (R_altrep_inherits(x, altptr_raw_class))
    {
        return true;
    }
    return false;
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
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    Rprintf("File type: %s, size: %llu, cache num: %llu\n",
            get_type_name(file_data.coerced_type).c_str(),
            (uint64_t)file_data.file_size,
            (uint64_t)file_data.write_cache.size());

    Rprintf("[Source info]\n");
    Rprintf("   Type: %s, length: %llu, private: %p",
            (uint64_t)get_type_name(file_data.altrep_info.type).c_str(),
            (uint64_t)file_data.altrep_info.length,
            file_data.altrep_info.private_data);
    if (file_data.altrep_info.operations.get_private_size != NULL)
    {
        size_t private_size = file_data.altrep_info.operations.get_private_size(&file_data.altrep_info);
        Rprintf(", private size:%llu\n", (uint64_t)private_size);
    }
    Rprintf(", protect: %p\n", file_data.altrep_info.protected_data);

    Rprintf("[Defined operations]\n");
    if (file_data.altrep_info.operations.get_region != 0)
        Rprintf("   get_region\n");
    if (file_data.altrep_info.operations.set_region != 0)
        Rprintf("   set_region\n");
    if (file_data.altrep_info.operations.extract_subset != 0)
        Rprintf("   extract_subset\n");
    if (file_data.altrep_info.operations.duplicate != 0)
        Rprintf("   duplicate\n");
    if (file_data.altrep_info.operations.coerce != 0)
        Rprintf("   coerce\n");
    if (file_data.altrep_info.operations.get_private_size != 0)
        Rprintf("   get_private_size\n");
    if (file_data.altrep_info.operations.inspect_private != 0)
        Rprintf("   inspect_private\n");
    if (file_data.altrep_info.operations.serialize != 0)
        Rprintf("   serialize\n");
    if (file_data.altrep_info.operations.unserialize != 0)
        Rprintf("   unserialize\n");

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
            Rprintf("   Cache block %llu, shared number %llu, ptr: %p\n", i.first, i.second.use_count(), i.second.get_const());
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
    Filesystem_file_data &old_file_data = get_filesystem_file_data(file_name);
    Travel_altrep_info &old_altrep_info = old_file_data.altrep_info;

    //Create a new virtual file
    Travel_altrep_info new_altrep_info;
    if (old_altrep_info.operations.duplicate != NULL)
    {
        altrep_print("Using customized duplicate method\n");
        new_altrep_info = old_altrep_info.operations.duplicate(&old_altrep_info);
    }
    if (old_altrep_info.operations.duplicate == NULL ||
        IS_ALTREP_INFO_VALID(new_altrep_info))
    {
        altrep_print("Using default duplicate method\n");
        new_altrep_info = old_altrep_info;
    }

    Filesystem_file_info new_file_info = add_filesystem_file(old_file_data.coerced_type,
                                                             old_file_data.index,
                                                             new_altrep_info);
    std::string &new_file_name = new_file_info.file_name;
    Filesystem_file_data &new_file_data = get_filesystem_file_data(new_file_name);

    //Copy write cache
    claim(old_file_data.cache_size == new_file_data.cache_size);
    new_file_data.write_cache = old_file_data.write_cache;

    //Duplicate the object
    SEXP res = Travel_make_altptr_internal(new_file_info);
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
    Filesystem_file_data &old_file_data = get_filesystem_file_data(file_name);
    Travel_altrep_info &old_altrep_info = old_file_data.altrep_info;

    //Create a new altrep info
    Travel_altrep_info new_altrep_info;
    if (old_altrep_info.operations.coerce != NULL)
    {
        altrep_print("Using the customized coercion method\n");
        new_altrep_info = old_altrep_info.operations.coerce(&old_file_data.altrep_info, type);
    }
    if (old_altrep_info.operations.coerce == NULL ||
        IS_ALTREP_INFO_VALID(new_altrep_info))
    {
        altrep_print("Using the default coercion method\n");
        new_altrep_info = old_altrep_info;
    }

    //Create a new virtual file
    Filesystem_file_info new_file_info = add_filesystem_file(type,
                                                             old_file_data.index,
                                                             new_altrep_info);
    std::string new_file_name = new_file_info.file_name;
    Filesystem_file_data &new_file_data = get_filesystem_file_data(new_file_name);

    //Convert the write_cach
    {
        Filesystem_cache_copier copier(new_file_data, old_file_data);
        size_t old_cache_element_num = old_file_data.cache_size / old_file_data.unit_size;
        for (const auto &i : old_file_data.write_cache)
        {
            size_t cache_offset = old_file_data.get_cache_offset(i.first);
            size_t cache_elt_offset = cache_offset / old_file_data.unit_size;
            for (size_t j = 0; j < old_cache_element_num; j++)
            {
                copier.copy(cache_elt_offset + j, cache_elt_offset + j);
            }
        }
    }
    //Make the new altrep
    SEXP res = Travel_make_altptr_internal(new_file_info);
    return res;
}

SEXP altptr_subset(SEXP x, SEXP idx, SEXP call)
{
    altrep_print("subsetting object\n");
    if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    //Pass all changes to the filesystem
    flush_altrep(x);
    //Get the file name
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &old_file_data = get_filesystem_file_data(file_name);
    Travel_altrep_info &old_altrep_info = old_file_data.altrep_info;

    //Create a new altrep info
    Travel_altrep_info new_altrep_info;
    if (old_altrep_info.operations.extract_subset != NULL)
    {
        altrep_print("Using the customized subset method\n");
        //If the old object has the subset function defined,
        //its offset and step should be 0 and 1 respectively
        claim(old_file_data.index.start == 0);
        claim(old_file_data.index.step == 1);
        claim(old_file_data.index.block_length == 1);
        new_altrep_info = old_altrep_info.operations.extract_subset(&old_file_data.altrep_info, idx);
    }

    Subset_index index(XLENGTH(idx));
    //If no subset method defined for the idx or
    //The method return an invalid altrep_info
    if (old_altrep_info.operations.extract_subset == NULL ||
        IS_ALTREP_INFO_VALID(new_altrep_info))
    {
        altrep_print("Using the default subset method\n");
        //Check if the index is an arithmetic sequence
        bool arithmetic = Subset_index::to_subset_index(idx, index, old_file_data.index);
        //If index is not an arithmetic sequence or the length of the subsetted vector
        //is less than cutoff, we return a regular vector
        if (!arithmetic || index.length < default_subset_length_cutoff)
        {
            SEXP x_new = PROTECT(Rf_allocVector(TYPEOF(x), XLENGTH(idx)));
            char *ptr_old = (char *)DATAPTR(x);
            char *ptr_new = (char *)DATAPTR(x_new);
            uint8_t &unit_size = old_file_data.unit_size;
            DO_BY_TYPE(idx_cast, idx, {
                for (size_t i = 0; i < index.length; i++)
                {
                    memcpy(ptr_new + i * unit_size, ptr_old + ((size_t)idx_cast[i]-1) * unit_size, unit_size);
                }
            })
            UNPROTECT(1);
            return x_new;
        }
        else
        {
            //If the index is an arithmetic sequence, we will use the default subset method for the sequence
            new_altrep_info = old_altrep_info;
        }
    }

    //Create a new virtual file
    Filesystem_file_info new_file_info = add_filesystem_file(old_file_data.coerced_type,
                                                             index,
                                                             new_altrep_info);
    std::string new_file_name = new_file_info.file_name;
    Filesystem_file_data &new_file_data = get_filesystem_file_data(new_file_name);

    //Copy cache
    {
        Filesystem_cache_copier copier(new_file_data, old_file_data);
        for (const auto &i : old_file_data.write_cache)
        {
            size_t cache_start_offset = old_file_data.get_cache_offset(i.first);
            size_t cache_start_elt = cache_start_offset / old_file_data.unit_size;
            size_t cache_size = get_file_read_size(old_file_data.file_size,
                                                   cache_start_offset,
                                                   old_file_data.cache_size);
            size_t cache_length = cache_size / old_file_data.unit_size;
            //This mighe be slow, but it should work
            for (size_t j = 0; j <= cache_length; j++)
            {
                //get the element index in the source file for the current cached element
                size_t source_elt_offset = old_file_data.index.get_source_index(cache_start_elt + j);
                //if the element may be in the subsetted vector
                if (source_elt_offset > index.start)
                {
                    //Check if the element in the subsetted vector
                    size_t dest_within_block_elt_offset = (source_elt_offset - index.start) % index.step;
                    if (dest_within_block_elt_offset < index.block_length)
                    {
                        size_t dest_elt_offset = new_file_data.index.get_subset_index(source_elt_offset);
                        copier.copy(dest_elt_offset, source_elt_offset);
                    }
                }
            }
        }
    }
    //Make the new altrep
    SEXP res = Travel_make_altptr_internal(new_file_info);
    return res;
}

SEXP altptr_serialized(SEXP x)
{
    altrep_print("serialize function\n");
    if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    //Pass all changes to the filesystem
    flush_altrep(x);
    //Get the file data
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    Travel_altrep_info &altrep_info = file_data.altrep_info;

    PROTECT_GUARD guard;
    SEXP serialized_object;
    if (altrep_info.operations.serialize == R_NilValue)
    {
        //If the serialize function is not set
        //We just pass a regular vector
        serialized_object = guard.protect(Rf_allocVector(file_data.coerced_type, file_data.file_length));
        memcpy(DATAPTR(serialized_object), DATAPTR(x), file_data.file_size);
    }
    else
    {
        //If the serialize function has been set,
        //we use the user-provided function to serialize
        //the altrep_info
        Rcpp::Function serialize_func(altrep_info.operations.serialize);
        SEXP altrep_info_extptr = guard.protect(R_MakeExternalPtr(&altrep_info, R_NilValue, R_NilValue));
        SEXP serialized_altrep_info = guard.protect(serialize_func(altrep_info_extptr));
        Exported_file_data exported_data = file_data.serialize();
        SEXP serialized_file_data = guard.protect(Rf_allocVector(RAWSXP, sizeof(Exported_file_data)));
        //prepare the file data
        void *file_ptr = (void *)DATAPTR(serialized_file_data);
        memcpy(file_ptr, &exported_data, sizeof(Exported_file_data));
        //prepare the cache data
        size_t cache_num = file_data.write_cache.size();
        size_t cache_size = file_data.cache_size;
        Rcpp::NumericVector cache_id(cache_num);
        Rcpp::RawVector cache_data(cache_num * file_data.cache_size);
        size_t j = 0;
        char *cache_ptr = (char *)DATAPTR(cache_data);
        for (const auto &i : file_data.write_cache)
        {
            cache_id[j] = i.first;
            memcpy(cache_ptr + j * cache_size, i.second.get_const(), cache_size);
            j++;
        }
        serialized_object = guard.protect(Rf_allocVector(VECSXP, 5));
        SET_VECTOR_ELT(serialized_object, 0, altrep_info.operations.unserialize);
        SET_VECTOR_ELT(serialized_object, 1, serialized_altrep_info);
        SET_VECTOR_ELT(serialized_object, 2, serialized_file_data);
        SET_VECTOR_ELT(serialized_object, 3, cache_id);
        SET_VECTOR_ELT(serialized_object, 4, cache_data);
    }
    return serialized_object;
}

SEXP altptr_unserialize(SEXP R_class, SEXP serialized_object)
{
    altrep_print("serialize function\n");
    //If the serialized object is a regular vector, we just return it.
    if (TYPEOF(serialized_object) != VECSXP)
    {
        return serialized_object;
    }
    //Otherwise, we build the ALTREP object from scratch
    if (!is_filesystem_running())
    {
        Rf_error("The filesystem is not running!\n");
    }
    Rcpp::List serialized_list = serialized_object;
    Rcpp::Function unserialize_func = serialized_list[0];
    PROTECT_GUARD guard;
    SEXP x = guard.protect(unserialize_func(serialized_list[1]));
    SEXP serialized_file_data = serialized_list[2];
    Exported_file_data *file_data_ptr = (Exported_file_data *)DATAPTR(serialized_file_data);
    Rcpp::NumericVector cache_id = serialized_list[3];
    Rcpp::RawVector cache_data = serialized_list[4];
    //If the length of the unserialized object does not match
    //the length of the orignal one, we directly return the object
    //with a warning
    if (file_data_ptr->source_length != (size_t)XLENGTH(x))
    {
        Rf_warning("The length of the unserialized object does not match the orignal one, "
                   "expected: %llu, true: %llu\n",
                   (uint64_t)file_data_ptr->source_length, (uint64_t)XLENGTH(x));
        return x;
    }
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
    R_set_altvec_Dataptr_or_null_method(ALT_CLASS, altptr_dataptr_or_null);\
	R_set_altvec_Extract_subset_method(ALT_CLASS, altptr_subset);

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
