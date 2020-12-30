#include <Rcpp.h>
#include <R_ext/Altrep.h>
#include "altrep_macros.h"
#include "altrep_manager.h"
#include "filesystem_manager.h"
#include "memory_mapped_file.h"
#include "read_write_operations.h"
#include "package_settings.h"
#include "utils.h"
#include "class_Filesystem_cache_copier.h"
#include "class_Timer.h"
#include "class_Protect_guard.h"


R_altrep_class_t altmmap_real_class;
R_altrep_class_t altmmap_integer_class;
R_altrep_class_t altmmap_logical_class;
R_altrep_class_t altmmap_raw_class;
//R_altrep_class_t altmmap_complex_class;

R_altrep_class_t get_altmmap_class(int type)
{
    switch (type)
    {
    case REALSXP:
        return altmmap_real_class;
    case INTSXP:
        return altmmap_integer_class;
    case LGLSXP:
        return altmmap_logical_class;
    case RAWSXP:
        return altmmap_raw_class;
    case CPLXSXP:
        //return altmmap_complex_class;
    case STRSXP:
        //return altmmap_str_class;
    default:
        Rf_error("Type of %d is not supported yet", type);
    }
    // Just for suppressing the annoying warning, it should never be excuted
    return altmmap_real_class;
}

bool is_altmmap(SEXP x)
{
    if (R_altrep_inherits(x, altmmap_real_class))
    {
        return true;
    }
    if (R_altrep_inherits(x, altmmap_integer_class))
    {
        return true;
    }
    if (R_altrep_inherits(x, altmmap_logical_class))
    {
        return true;
    }
    if (R_altrep_inherits(x, altmmap_raw_class))
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

static Rboolean altmmap_Inspect(SEXP x, int pre, int deep, int pvec,
                         void (*inspect_subtree)(SEXP, int, int, int))
{
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    Rprintf("File type: %s, size: %llu, length:%llu, cache num: %llu\n",
            get_type_name(file_data.coerced_type).c_str(),
            (uint64_t)file_data.file_size,
            (uint64_t)file_data.file_length,
            (uint64_t)file_data.write_cache.size());
    Rprintf("[Source info]\n");
    Rprintf("   Type: %s, length: %llu, private: %p",
            (uint64_t)get_type_name(file_data.altrep_info.type).c_str(),
            (uint64_t)file_data.altrep_info.length,
            file_data.altrep_info.private_data);
    if (file_data.altrep_info.operations.get_private_size != NULL)
    {
        size_t private_size = file_data.altrep_info.operations.get_private_size(&file_data.altrep_info);
        Rprintf(", private size:%llu", (uint64_t)private_size);
    }
    if (file_data.altrep_info.protected_data != R_NilValue)
    {
        Rprintf(", protect: %p", file_data.altrep_info.protected_data);
    }
    else
    {
        Rprintf(", protect: %p", nullptr);
    }
    Rprintf("\n");

    Rprintf("[Index info]\n");
    Rprintf("   %s\n", file_data.index.summarize(4).c_str());

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
    if (file_data.altrep_info.operations.serialize != NULL)
        Rprintf("   serialize\n");
    if (file_data.altrep_info.operations.unserialize != R_NilValue)
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

static R_xlen_t altmmap_length(SEXP x)
{
    R_xlen_t size = Rcpp::as<R_xlen_t>(GET_ALT_LENGTH(x));
    altrep_print("accessing length: %llu\n", size);
    return size;
}

static void *altmmap_dataptr(SEXP x, Rboolean writeable)
{
    altrep_print("accessing data pointer\n");
    Memory_mapped *handle = GET_ALT_FILE_HANDLE(x);
    if (!handle->is_mapped())
    {
        Rf_error("The file handle has been released!\n");
    }
    return handle->get_ptr();
}

static const void *altmmap_dataptr_or_null(SEXP x)
{
    altrep_print("accessing data pointer or null\n");
    return altmmap_dataptr(x, Rboolean::TRUE);
}


/*
static unsigned char altmmap_get_raw_elt(SEXP x, R_xlen_t i){
    altrep_print("Accessing elt at %llu\n", (unsigned long long)i);
    unsigned char result;
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    general_read_func(file_data, &result, i*sizeof(result), sizeof(result));
    return result;
}
static int altmmap_get_int_elt(SEXP x, R_xlen_t i){
    altrep_print("Accessing elt at %llu\n", (unsigned long long)i);
    int result;
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    general_read_func(file_data, &result, i*sizeof(result), sizeof(result));
    return result;
}
static double altmmap_get_numeric_elt(SEXP x, R_xlen_t i){
    altrep_print("Accessing elt at %llu\n", (unsigned long long)i);
    double result;
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    general_read_func(file_data, &result, i*sizeof(result), sizeof(result));
    return result;
}
*/


static SEXP altmmap_duplicate(SEXP x, Rboolean deep)
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

    Filesystem_file_identifier new_file_info = add_filesystem_file(old_file_data.coerced_type,
                                                                   old_file_data.index,
                                                                   new_altrep_info);
    std::string &new_file_name = new_file_info.file_name;
    Filesystem_file_data &new_file_data = get_filesystem_file_data(new_file_name);

    //Copy write cache
    assert(old_file_data.cache_size == new_file_data.cache_size);
    new_file_data.write_cache = old_file_data.write_cache;

    //Duplicate the object
    Protect_guard guard;
    SEXP res = guard.protect(Travel_make_altmmap(new_file_info));
    //SHALLOW_DUPLICATE_ATTRIB(res, x);
    return res;
}

static SEXP altmmap_coerce(SEXP x, int type)
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
    Filesystem_file_identifier new_file_info = add_filesystem_file(type,
                                                                   old_file_data.index,
                                                                   new_altrep_info);
    std::string new_file_name = new_file_info.file_name;
    Filesystem_file_data &new_file_data = get_filesystem_file_data(new_file_name);

    //Copy cache
    {
        Filesystem_cache_copier copier(new_file_data, old_file_data);
        Filesystem_cache_index_iterator old_cache_idx_iter = old_file_data.get_cache_iterator();
        for (; !old_cache_idx_iter.is_final(); ++old_cache_idx_iter)
        {
            //get the index in the source file for the cached element
            size_t source_elt_offset = old_cache_idx_iter.get_index_in_source();
            copier.copy(source_elt_offset, source_elt_offset);
        }
    }
    //Make the new altrep
    Protect_guard guard;
    SEXP res = guard.protect(Travel_make_altmmap(new_file_info));
    //SHALLOW_DUPLICATE_ATTRIB(res, x);
    return res;
}



static SEXP altmmap_subset(SEXP x, SEXP idx, SEXP call)
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
    Subset_index &old_index = old_file_data.index;

    //Create a new altrep info
    Travel_altrep_info new_altrep_info;
    if (old_altrep_info.operations.extract_subset != NULL)
    {
        altrep_print("Using the customized subset method\n");
        //If the old object has the subset function defined,
        //its offset and stride should be 0 and 1 respectively
        if (old_index.starts.size() == 1 &&
            old_index.starts[0] == 0 &&
            old_index.strides[0] == 1 &&
            old_index.total_length == (size_t)XLENGTH(x))
        {
            new_altrep_info = old_altrep_info.operations.extract_subset(&old_file_data.altrep_info, idx);
        }
        else
        {
            altrep_print("The data has been sampled, use the default method instead\n");
        }
    }

    Subset_index new_index;
    //If no subset method defined for the idx or
    //The method return an invalid altrep_info
    if (old_altrep_info.operations.extract_subset == NULL ||
        IS_ALTREP_INFO_VALID(new_altrep_info))
    {
        altrep_print("Using the default subset method\n");
        size_t index_length = XLENGTH(idx);
        size_t subset_index_size = Subset_index::get_index_size(idx, old_index);
        //Check if the index is an arithmetic sequence
        new_index = Subset_index::to_subset_index(idx, old_file_data.index);
        //If the size of the raw index is less than the size of the Subset_index object,
        //we return a regular vector
        if (index_length < subset_index_size)
        {
            SEXP x_new = PROTECT(Rf_allocVector(TYPEOF(x), XLENGTH(idx)));
            char *ptr_old = (char *)DATAPTR(x);
            char *ptr_new = (char *)DATAPTR(x_new);
            uint8_t &unit_size = old_file_data.unit_size;
            for (size_t i = 0; i < index_length; i++)
            {
                memcpy(ptr_new + i * unit_size, ptr_old + GET_INDEX(idx, i) * unit_size, unit_size);
            }
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
    Filesystem_file_identifier new_file_info = add_filesystem_file(old_file_data.coerced_type,
                                                                   new_index,
                                                                   new_altrep_info);
    std::string new_file_name = new_file_info.file_name;
    Filesystem_file_data &new_file_data = get_filesystem_file_data(new_file_name);

    //Copy cache
    {
        Filesystem_cache_copier copier(new_file_data, old_file_data);
        Filesystem_cache_index_iterator old_cache_idx_iter = old_file_data.get_cache_iterator();
        for (; !old_cache_idx_iter.is_final(); ++old_cache_idx_iter)
        {
            //get the index in the source file for the cached element
            size_t source_elt_offset = old_cache_idx_iter.get_index_in_source();
            //if the element is in the subsetted vector
            if (new_index.contains_index(source_elt_offset))
            {
                std::vector<size_t> dest_elt_offset = new_index.get_subset_index(source_elt_offset);
                for(const auto& i:dest_elt_offset){
                    copier.copy(i, source_elt_offset);
                }
            }
        }
    }
    //Make the new altrep
    SEXP res = Travel_make_altmmap(new_file_info);
    return res;
}

static SEXP altmmap_serialize(SEXP x)
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

    Protect_guard guard;
    SEXP serialized_object;
    if (altrep_info.operations.serialize == NULL)
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
        SEXP serialized_altrep_info = guard.protect(altrep_info.operations.serialize(&altrep_info));
        size_t serialized_file_data_size = file_data.get_serialize_size();
        SEXP serialized_file_data = guard.protect(Rf_allocVector(RAWSXP, serialized_file_data_size));
        file_data.serialize(DATAPTR(serialized_file_data));
        serialized_object = guard.protect(Rf_allocVector(VECSXP, 3));
        SET_VECTOR_ELT(serialized_object, 0, altrep_info.operations.unserialize);
        SET_VECTOR_ELT(serialized_object, 1, serialized_altrep_info);
        SET_VECTOR_ELT(serialized_object, 2, serialized_file_data);
    }
    return serialized_object;
}

static SEXP altmmap_unserialize(SEXP R_class, SEXP serialized_object)
{
    altrep_print("serialize function\n");
    //If the serialized object is a regular vector, we just return it.
    if (TYPEOF(serialized_object) != VECSXP)
    {
        return serialized_object;
    }
    //Otherwise, we build the ALTREP object from scratch
    Rcpp::List serialized_list = serialized_object;
    Rcpp::Function unserialize_func = serialized_list[0];
    Protect_guard guard;
    SEXP x = guard.protect(unserialize_func(serialized_list[1]));

    //update the filesystem data;
    SEXP serialized_file_data = serialized_list[2];
    std::string file_name = Rcpp::as<std::string>(GET_ALT_NAME(x));
    Filesystem_file_data &file_data = get_filesystem_file_data(file_name);
    file_data.unserialize(DATAPTR(serialized_file_data));
    return x;
}

/*
Register ALTREP class
*/

#define ALT_COMMOM_REGISTRATION(ALT_CLASS, ALT_TYPE, FUNC_PREFIX)            \
    ALT_CLASS = R_make_##ALT_TYPE##_class(class_name, PACKAGE_NAME, dll);    \
    /* common ALTREP methods */                                              \
    R_set_altrep_Inspect_method(ALT_CLASS, altmmap_Inspect);                 \
    R_set_altrep_Length_method(ALT_CLASS, altmmap_length);                   \
    R_set_altrep_Duplicate_method(ALT_CLASS, altmmap_duplicate);             \
    R_set_altrep_Coerce_method(ALT_CLASS, altmmap_coerce);                   \
    R_set_altrep_Serialized_state_method(ALT_CLASS, altmmap_serialize);      \
    R_set_altrep_Unserialize_method(ALT_CLASS, altmmap_unserialize);         \
    /* ALTVEC methods */                                                     \
    R_set_altvec_Dataptr_method(ALT_CLASS, altmmap_dataptr);                 \
    R_set_altvec_Dataptr_or_null_method(ALT_CLASS, altmmap_dataptr_or_null); \
    R_set_altvec_Extract_subset_method(ALT_CLASS, altmmap_subset);

//R_set_altvec_Extract_subset_method(ALT_CLASS, numeric_subset<R_TYPE, C_TYPE>);

//[[Rcpp::init]]
void init_altmmap_logical_class(DllInfo *dll)
{
    char class_name[] = "travel_altmmap_logical";
    ALT_COMMOM_REGISTRATION(altmmap_logical_class, altlogical, LOGICAL);
    //R_set_altlogical_Elt_method(altmmap_logical_class,altmmap_get_int_elt);
}
//[[Rcpp::init]]
void init_altmmap_integer_class(DllInfo *dll)
{
    char class_name[] = "travel_altmmap_integer";
    ALT_COMMOM_REGISTRATION(altmmap_integer_class, altinteger, INTEGER);
    //R_set_altinteger_Elt_method(altmmap_integer_class,altmmap_get_int_elt);
}

//[[Rcpp::init]]
void ini_altmmap_real_class(DllInfo *dll)
{
    char class_name[] = "travel_altmmap_real";
    ALT_COMMOM_REGISTRATION(altmmap_real_class, altreal, REAL);
    //R_set_altreal_Elt_method(altmmap_real_class,altmmap_get_numeric_elt);
}

//[[Rcpp::init]]
void init_altmmap_raw_class(DllInfo *dll)
{
    char class_name[] = "travel_altmmap_raw";
    ALT_COMMOM_REGISTRATION(altmmap_raw_class, altraw, RAW);
    //R_set_altraw_Elt_method(altmmap_raw_class,altmmap_get_raw_elt);
}
