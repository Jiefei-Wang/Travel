#include "altrep.h"
#include "utils.h"
#include "filesystem_manager.h"
#include "memory_mapped_file.h"
#include "package_settings.h"
#include "Travel.h"
#include "read_write_operations.h"

size_t subset_cutoff = 3;

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

/*
A class to copy cached data from old file to new file
*/
class Filesystem_cache_copier
{
    Filesystem_file_data &dest_file_info;
    Filesystem_file_data &source_file_info;
    size_t dest_unit_size;
    size_t source_unit_size;
    //the offset of the buffer data in the destination file
    size_t buffer_offset;
    size_t buffer_size = 0;
    std::unique_ptr<char[]> buffer;

public:
    Filesystem_cache_copier(Filesystem_file_data &source_file_info, Filesystem_file_data &dest_file_info) : source_file_info(source_file_info), dest_file_info(dest_file_info)
    {
        source_unit_size = get_type_size(source_file_info.coerced_type);
        dest_unit_size = get_type_size(dest_file_info.coerced_type);
    }
    void copy(size_t dest_index, size_t source_index, Cache_block &source_cache)
    {
        //The offset of the data in the source file measured in byte
        size_t source_data_offset = source_index * source_unit_size;
        //The offset of the beginning of the cache block in the source file measured in byte
        size_t source_cache_offset = source_data_offset / dest_file_info.cache_size * dest_file_info.cache_size;
        size_t dest_data_offset = dest_index * dest_unit_size;
        size_t dest_cache_offset = dest_data_offset / dest_file_info.cache_size * dest_file_info.cache_size;
        if (buffer_size != 0)
        {
            if (buffer_offset != dest_cache_offset)
            {
                flush_buffer();
            }
        }
        else
        {
            RESERVE_BUFFER(buffer, buffer_size, dest_file_info.cache_size);
            general_read_func(source_file_info, buffer.get(),
                              dest_cache_offset,
                              dest_file_info.cache_size);
        }
        buffer_offset = dest_cache_offset;
        //The offset of the data within the cache measured in bytes
        size_t source_within_cache_offset = source_data_offset-source_cache_offset;
        //copy_memory();
    }
    void flush_buffer()
    {
    }
};

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

    Filesystem_file_info new_file_info = add_filesystem_file(old_file_data.coerced_type, new_altrep_info);
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
    Filesystem_file_info new_file_info = add_filesystem_file(type, new_altrep_info);
    std::string new_file_name = new_file_info.file_name;
    Filesystem_file_data &new_file_data = get_filesystem_file_data(new_file_name);

    //Convert the write_cache
    size_t old_file_unit_size = get_type_size(old_file_data.coerced_type);
    size_t new_file_unit_size = get_type_size(new_file_data.coerced_type);
    size_t buffer_size = std::max(
        old_file_data.cache_size,
        old_file_data.cache_size / old_file_unit_size * new_file_unit_size);
    std::unique_ptr<char[]> buffer(new char[buffer_size]);
    for (const auto &i : old_file_data.write_cache)
    {
        size_t read_offset = i.first * old_file_data.cache_size;
        size_t read_size = get_file_read_size(old_file_data.file_size, read_offset, old_file_data.cache_size);
        general_read_func(old_file_data, buffer.get(), read_offset, read_size);
        copy_memory(new_file_data.coerced_type, old_file_data.coerced_type,
                    buffer.get(), buffer.get(),
                    old_file_data.cache_size / old_file_unit_size,
                    new_file_unit_size > old_file_unit_size);
        size_t write_offset = read_offset / old_file_unit_size * new_file_unit_size;
        size_t write_size = read_size / old_file_unit_size * new_file_unit_size;
        general_write_func(new_file_data, buffer.get(), write_offset, write_size);
    }
    //Make the new altrep
    SEXP res = Travel_make_altptr_internal(new_file_info);
    return res;
}

#define DO_BY_TYPE(x_cast, x, OPERATION)                 \
    switch (TYPEOF(x))                                   \
    {                                                    \
    case INTSXP:                                         \
    {                                                    \
        Rcpp::IntegerVector x_cast = x;                  \
        OPERATION;                                       \
        break;                                           \
    }                                                    \
    case REALSXP:                                        \
    {                                                    \
        Rcpp::NumericVector x_cast = x;                  \
        OPERATION;                                       \
        break;                                           \
    }                                                    \
    default:                                             \
        Rf_error("Unexpected index type %d", TYPEOF(x)); \
    }

//Check if the index is an arithmetic sequence
bool is_index_arithmetic_seq(SEXP idx, size_t &start_off, size_t &step)
{
    if (XLENGTH(idx) <= 1)
    {
        return false;
    }
    DO_BY_TYPE(index, idx, {
        start_off = index[0];
        step = index[1] - index[0];
        for (size_t i = 2; i < index.length(); i++)
        {
            if (index[i] != start_off + step * i)
            {
                return false;
            }
        }
    })
    return true;
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
        claim(old_file_data.start_offset == 0);
        claim(old_file_data.step == 1);
        new_altrep_info = old_altrep_info.operations.extract_subset(&old_file_data.altrep_info, idx);
    }

    size_t start_offset;
    size_t step;
    bool arithmetic;
    bool default_subset_method = false;
    //If no subset method defined for the idx
    if (old_altrep_info.operations.extract_subset == NULL ||
        IS_ALTREP_INFO_VALID(new_altrep_info))
    {
        altrep_print("Using the default subset method\n");
        //Check if the index is an arithmetic sequence
        arithmetic = is_index_arithmetic_seq(idx, start_offset, step);
        //If index is not an arithmetic sequence or the length of the subsetted vector
        //is less than cutoff, we return a regular vector
        if (!arithmetic || XLENGTH(idx) < subset_cutoff)
        {
            SEXP x_new = PROTECT(Rf_allocVector(TYPEOF(x), XLENGTH(idx)));
            char *ptr_old = (char *)DATAPTR(x);
            char *ptr_new = (char *)DATAPTR(x_new);
            const size_t unit_size = get_type_size(old_file_data.coerced_type);
            DO_BY_TYPE(index, idx, {
                for (size_t i = 0; i < index.length(); i++)
                {
                    memcpy(ptr_new + i * unit_size, ptr_new + ((size_t)index[i]) * unit_size, unit_size);
                }
            })
            UNPROTECT(1);
            return x_new;
        }
        else
        {
            //If the index is an arithmetic sequence, we will use the default subset method for sequence
            new_altrep_info = old_altrep_info;
            default_subset_method = true;
        }
    }

    //Create a new virtual file
    Filesystem_file_info new_file_info = add_filesystem_file(old_file_data.coerced_type,
                                                             new_altrep_info);
    std::string new_file_name = new_file_info.file_name;
    Filesystem_file_data &new_file_data = get_filesystem_file_data(new_file_name);
    const size_t unit_size = get_type_size(new_file_data.coerced_type);
    //Create the write cache for the new object
    const size_t cache_size = new_file_data.cache_size;

    //set the length, start offset and step
    if (default_subset_method)
    {
        new_file_data.start_offset = start_offset;
        new_file_data.step = step;
        new_file_data.file_size = XLENGTH(idx) * unit_size;
    }

    //Copy cache
    size_t buffer_size = 0;
    std::unique_ptr<char[]> buffer;
    for (const auto &i : old_file_data.write_cache)
    {
        size_t cache_start_offset = i.first * old_file_data.cache_size;
        size_t cache_start_elt = cache_start_offset / unit_size;
        size_t cache_end_offset = get_file_read_size(old_file_data.file_size,
                                                     cache_start_offset, cache_size);
        size_t cache_end_elt = cache_end_offset / unit_size;
        if (cache_end_elt < start_offset)
        {
            continue;
        }
        size_t min_start_source_elt = std::max(cache_start_elt, start_offset);
        size_t first_cpy_dest_elt = round_up_division(min_start_source_elt - start_offset, step);
        size_t first_cpy_source_elt = first_cpy_dest_elt * step + start_offset;
        size_t first_cpy_cache_elt = first_cpy_source_elt - cache_start_elt;
        size_t n_cpy = round_up_division(zero_bounded_minus(cache_end_elt, first_cpy_cache_elt), step);
        RESERVE_BUFFER(buffer, buffer_size, n_cpy * unit_size);
        for (size_t j = 0; j < n_cpy; j = j++)
        {
            general_read_func(old_file_data, buffer.get() + j * unit_size,
                              (first_cpy_source_elt + j * step) * unit_size, unit_size);
        }
        general_write_func(new_file_data, buffer.get(), first_cpy_dest_elt * unit_size, n_cpy * unit_size);
    }
    //Make the new altrep
    SEXP res = Travel_make_altptr_internal(new_file_info);
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
