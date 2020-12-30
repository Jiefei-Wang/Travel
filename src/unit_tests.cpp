#include <Rcpp.h>
#include "Travel.h"
//#include "utils.h"
#include "filesystem_manager.h"
#include "altrep_manager.h"
#include "class_Unique_buffer.h"
#include "utils.h"
#include "unit_test_utils.h"
#include "class_Protect_guard.h"
/*
=========================================================================================
                          Subset_index
=========================================================================================
*/
//[[Rcpp::export]]
void C_test_Subset_index_basic()
{
    Subset_index index;
    throw_if_not(index.total_length == 0);
    throw_if_not(index.is_consecutive());

    size_t total_length = 0;
    //Add an index
    size_t start = 1;
    size_t length = 10;
    size_t stride = 1;
    total_length += length;
    index.push_back(start, length, stride);
    throw_if_not(index.total_length == total_length);
    throw_if_not(index.is_consecutive());

    //Merge an index
    start = 11;
    length = 10;
    stride = 1;
    total_length += length;
    index.push_back(start, length, stride);
    throw_if_not(index.total_length == total_length);
    throw_if_not(index.is_consecutive());
    throw_if_not(index.partial_lengths.size() == 1);
    throw_if_not(index.starts[0] == 1);
    throw_if_not(index.lengths[0] == 20);
    throw_if_not(index.strides[0] == 1);
    throw_if_not(index.partial_lengths[0] == 0);

    //Merge an index with overlap
    start = 7;
    length = 6;
    stride = 1;
    total_length += length;
    index.push_back(start, length, stride);
    throw_if_not(index.total_length == total_length);
    throw_if_not(!index.is_consecutive());
    throw_if_not(index.partial_lengths.size() == 2);
    throw_if_not(index.starts[0] == 1);
    throw_if_not(index.lengths[0] == 20);
    throw_if_not(index.strides[0] == 1);
    throw_if_not(index.partial_lengths[0] == 0);
    throw_if_not(index.starts[1] == 7);
    throw_if_not(index.lengths[1] == 6);
    throw_if_not(index.strides[1] == 1);
    throw_if_not(index.partial_lengths[1] == 20);

    //add a non-consecutive index
    start = 31;
    length = 10;
    stride = 2;
    total_length += length;
    index.push_back(start, length, stride);
    throw_if_not(index.total_length == total_length);
    throw_if_not(!index.is_consecutive());
    throw_if_not(index.partial_lengths.size() == 3);
    throw_if_not(index.starts[0] == 1);
    throw_if_not(index.lengths[0] == 20);
    throw_if_not(index.strides[0] == 1);
    throw_if_not(index.partial_lengths[0] == 0);
    throw_if_not(index.starts[1] == 7);
    throw_if_not(index.lengths[1] == 6);
    throw_if_not(index.strides[1] == 1);
    throw_if_not(index.partial_lengths[1] == 20);
    throw_if_not(index.starts[2] == 31);
    throw_if_not(index.lengths[2] == 10);
    throw_if_not(index.strides[2] == 2);
    throw_if_not(index.partial_lengths[2] == 26);
}

//[[Rcpp::export]]
void C_test_Subset_index_conversion()
{
    Rcpp::NumericVector idx1 = Rcpp::NumericVector::create(1, 2, 3, 4, 6, 7, 8, 9, 11, 13, 15, 17, 18);
    Subset_index ref_index1(0, 100);
    Subset_index index1 = Subset_index::to_subset_index(idx1, ref_index1);
    throw_if_not(index1.total_length == (size_t)idx1.length());
    throw_if_not(index1.starts[0] == 0);
    throw_if_not(index1.starts[1] == 5);
    throw_if_not(index1.starts[2] == 10);
    throw_if_not(index1.starts[3] == 17);
    throw_if_not(index1.lengths[0] == 4);
    throw_if_not(index1.lengths[1] == 4);
    throw_if_not(index1.lengths[2] == 4);
    throw_if_not(index1.lengths[3] == 1);
    throw_if_not(index1.strides[0] == 1);
    throw_if_not(index1.strides[1] == 1);
    throw_if_not(index1.strides[2] == 2);
    throw_if_not(index1.strides[3] == 1);
    throw_if_not(index1.partial_lengths[0] == 0);
    throw_if_not(index1.partial_lengths[1] == 4);
    throw_if_not(index1.partial_lengths[2] == 8);
    throw_if_not(index1.partial_lengths[3] == 12);
    for (size_t i = 0; i < index1.total_length; i++)
    {
        throw_if_not(index1.get_source_index(i) == idx1[i] - 1);
    }

    //Unordered subset
    Rcpp::NumericVector idx2 = Rcpp::NumericVector::create(6, 7, 8, 9, 1, 2, 3, 4);
    Subset_index ref_index2(0, 100);
    Subset_index index2 = Subset_index::to_subset_index(idx2, ref_index2);
    throw_if_not(index2.total_length == (size_t)idx2.length());
    throw_if_not(index2.starts[0] == 5);
    throw_if_not(index2.starts[1] == 0);
    throw_if_not(index2.lengths[0] == 4);
    throw_if_not(index2.lengths[1] == 4);
    throw_if_not(index2.strides[0] == 1);
    throw_if_not(index2.strides[1] == 1);
    throw_if_not(index2.partial_lengths[0] == 0);
    throw_if_not(index2.partial_lengths[1] == 4);
    for (size_t i = 0; i < index2.total_length; i++)
    {
        throw_if_not(index2.get_source_index(i) == idx2[i] - 1);
    }
    //subset with reference
    //c(7,8,1,3,4)
    Rcpp::NumericVector idx3 = Rcpp::NumericVector::create(2, 3, 5, 7, 8);
    Subset_index index3 = Subset_index::to_subset_index(idx3, index2);
    throw_if_not(index3.total_length == (size_t)idx3.length());
    throw_if_not(index3.starts[0] == 6);
    throw_if_not(index3.starts[1] == 0);
    throw_if_not(index3.starts[2] == 3);
    throw_if_not(index3.lengths[0] == 2);
    throw_if_not(index3.lengths[1] == 2);
    throw_if_not(index3.lengths[2] == 1);
    throw_if_not(index3.strides[0] == 1);
    throw_if_not(index3.strides[1] == 2);
    throw_if_not(index3.strides[2] == 1);
    throw_if_not(index3.partial_lengths[0] == 0);
    throw_if_not(index3.partial_lengths[1] == 2);
    throw_if_not(index3.partial_lengths[2] == 4);
    for (size_t i = 0; i < index3.total_length; i++)
    {
        throw_if_not(index3.get_source_index(i) == idx2[idx3[i] - 1] - 1);
    }
}

/*
=========================================================================================
                          Cache_block
=========================================================================================
*/
//[[Rcpp::export]]
void C_test_Cache_block()
{
    size_t cache_size = 1024;
    Cache_block cache1(cache_size);
    void *cache_ptr = cache1.get();
    throw_if_not(cache1.get_size() >= cache_size);
    throw_if_not(cache1.use_count() == 1);
    throw_if_not(cache1.get_const() == cache_ptr);

    cache1 = cache1;
    throw_if_not(cache1.get_size() >= cache_size);
    throw_if_not(cache1.use_count() == 1);
    throw_if_not(cache1.get_const() == cache_ptr);

    {
        Cache_block cache2 = cache1;
        throw_if_not(cache2.get_size() >= cache_size);
        throw_if_not(cache1.use_count() == 2);
        throw_if_not(cache2.use_count() == 2);
        throw_if_not(cache1.get_const() == cache_ptr);
        throw_if_not(cache2.get_const() == cache_ptr);
        throw_if(cache1.get() == cache_ptr);
        throw_if_not(cache2.get() == cache_ptr);
        throw_if_not(cache1.use_count() == 1);
        throw_if_not(cache2.use_count() == 1);
        cache_ptr = cache1.get();
    }

    {
        Cache_block cache3(cache1);
        throw_if_not(cache3.get_size() >= cache_size);
        throw_if_not(cache1.use_count() == 2);
        throw_if_not(cache3.use_count() == 2);
        throw_if_not(cache1.get_const() == cache_ptr);
        throw_if_not(cache3.get_const() == cache_ptr);
        throw_if(cache1.get() == cache_ptr);
        throw_if_not(cache3.get() == cache_ptr);
        throw_if_not(cache1.use_count() == 1);
        throw_if_not(cache3.use_count() == 1);
    }
}
/*
=========================================================================================
                          unit test for read write layers
=========================================================================================
*/
#include "read_write_operations.h"
/*
cache layer: read_data_with_cache
source alignment layer: read_with_alignment
coercion layer: read_source_with_coercion
source offset layer: read_source_with_subset
source layer: read_contiguous_data/read_data_by_block
*/
size_t read_data_with_cache(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t size);
size_t read_with_alignment(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t size);
size_t read_source_with_coercion(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t length);
size_t read_source_with_subset(Filesystem_file_data &file_data, char *buffer, size_t offset, size_t length);

/*
=========================================================================================
                          unit test for read_source_with_subset
=========================================================================================
*/
void test_read_source_with_subset_internal(Subset_index index)
{
    int type = INTSXP;
    size_t type_size = get_type_size(type);
    size_t length = index.total_length;
    std::unique_ptr<char[]> data(new char[type_size * length]);
    int *ptr = (int *)data.get();
    fill_int_seq_data(ptr, index);
    Filesystem_file_data &file_data = make_int_sequence_file(type, index);

    Unique_buffer buffer;
    for (size_t i = 0; i < length; i++)
    {
        size_t read_length = (size_t)R::runif(1, 100);
        if (i + read_length > length)
        {
            read_length = length - i;
        }
        size_t read_size = read_length * type_size;
        buffer.reserve(read_size);
        size_t true_read_len = read_source_with_subset(file_data, buffer.get(), i, read_length);
        throw_if(true_read_len != read_length);
        throw_if(memcmp(buffer.get(), ptr + i, read_size));
    }
}

// [[Rcpp::export]]
void C_test_int_read_source_with_subset()
{
    size_t length = 1024 * 1024;
    Subset_index index(0, length);
    test_read_source_with_subset_internal(index);
}

// [[Rcpp::export]]
void C_test_int_sub_read_source_with_subset()
{
    size_t length = 1024 * 1024;
    Subset_index index1;
    for (size_t i = 0; i < 10; i++)
    {
        size_t start = (size_t)R::runif(1, length);
        size_t sub_length = (size_t)R::runif(1, length);
        size_t stride = R::rnbinom(2, 0.7);
        index1.push_back(start, sub_length, stride);
    }
    test_read_source_with_subset_internal(index1);
}
/*
=========================================================================================
                          unit test for read_with_alignment
=========================================================================================
*/
template <class T>
void test_read_with_alignment_internal(int type, Subset_index index)
{
    size_t type_size = get_type_size(type);
    size_t length = index.total_length;
    size_t file_size = type_size * length;
    std::unique_ptr<char[]> data(new char[file_size]);
    T *ptr = (T *)data.get();
    fill_int_seq_data(ptr, index);
    Filesystem_file_data &file_data = make_int_sequence_file(type, index);

    Unique_buffer buffer;
    for (size_t i = 0; i < file_size; i++)
    {
        size_t read_size = (size_t)R::runif(1, 100);
        if (i + read_size > file_size)
        {
            read_size = file_size - i;
        }
        buffer.reserve(read_size);
        size_t true_read_size = read_with_alignment(file_data, buffer.get(), i, read_size);
        throw_if(true_read_size != read_size);
        throw_if(memcmp(buffer.get(), ((char *)ptr) + i, read_size));
    }
}

// [[Rcpp::export]]
void C_test_int_read_with_alignment()
{
    size_t length = 1024 * 1024;
    Subset_index index(0, length);
    test_read_with_alignment_internal<int>(INTSXP, index);
}

// [[Rcpp::export]]
void C_test_int_sub_read_with_alignment()
{
    size_t length = 1024 * 1024;
    Subset_index index1;
    for (size_t i = 0; i < 10; i++)
    {
        size_t start = (size_t)R::runif(1, length);
        size_t sub_length = (size_t)R::runif(1, 1024);
        size_t stride = R::rnbinom(2, 0.7);
        index1.push_back(start, sub_length, stride);
    }
    test_read_with_alignment_internal<int>(INTSXP, index1);
}
// [[Rcpp::export]]
void C_test_real_read_with_alignment()
{
    size_t length = 1024 * 1024;
    Subset_index index(0, length);
    test_read_with_alignment_internal<double>(REALSXP, index);
}

// [[Rcpp::export]]
void C_test_real_sub_read_with_alignment()
{
    size_t length = 1024 * 1024;
    Subset_index index1;
    for (size_t i = 0; i < 10; i++)
    {
        size_t start = (size_t)R::runif(1, length);
        size_t sub_length = (size_t)R::runif(1, 1024);
        size_t stride = R::rnbinom(2, 0.7);
        index1.push_back(start, sub_length, stride);
    }
    test_read_with_alignment_internal<double>(REALSXP, index1);
}

/*
=========================================================================================
                          unit test for general read/write functions
=========================================================================================
*/
template <class T>
void test_read_write_functions_internal(
    int type, size_t length, Subset_index index,
    Rcpp::NumericVector write_starts, Rcpp::NumericVector write_length,
    Rcpp::NumericVector read_starts, Rcpp::NumericVector read_length)
{
    Filesystem_file_data &file_data = make_int_sequence_file(type, index);
    uint8_t &type_size = file_data.unit_size;
    throw_if_not(file_data.file_length == index.total_length);
    //Create the test data
    size_t data_length = file_data.file_length;
    std::unique_ptr<char[]> data(new char[type_size * data_length]);
    T *data_ptr = (T *)data.get();
    fill_int_seq_data(data_ptr, index);

    //The data that is written is the negative value of the original value in the file
    for (R_xlen_t i = 0; i < write_starts.length(); i++)
    {
        size_t elt_offset = write_starts[i];
        size_t offset = elt_offset * type_size;
        size_t size = write_length[i] * type_size;
        for (size_t j = 0; j < write_length[i]; j++)
        {
            data_ptr[elt_offset + j] = -index.get_source_index(elt_offset + j);
        }
        size_t true_write_size = general_write_func(file_data, data_ptr + elt_offset, offset, size);
        throw_if(true_write_size != size);
    }
    //Test the read function by reading the data from the file
    Unique_buffer buffer;
    for (R_xlen_t i = 0; i < read_starts.length(); i++)
    {
        size_t elt_offset = read_starts[i];
        size_t offset = elt_offset * type_size;
        size_t size = read_length[i] * type_size;
        buffer.reserve(size);
        char *ptr = buffer.get();
        size_t true_read_size = general_read_func(file_data, ptr, offset, size);
        throw_if(true_read_size != size);
        throw_if(memcmp(buffer.get(), data_ptr + elt_offset, size));
    }
    if (file_data.write_cache.size() == 0)
    {
        Rf_error("The write cache seems untouched.");
    }
}

// [[Rcpp::export]]
void C_test_read_write_functions_native(
    size_t length,
    Rcpp::NumericVector write_starts, Rcpp::NumericVector write_length,
    Rcpp::NumericVector read_starts, Rcpp::NumericVector read_length)
{
    Subset_index index(0, length);
    test_read_write_functions_internal<int>(
        INTSXP, length, index,
        write_starts, write_length,
        read_starts, read_length);
}

// [[Rcpp::export]]
void C_test_read_write_functions_with_coercion(
    size_t length,
    Rcpp::NumericVector write_starts, Rcpp::NumericVector write_length,
    Rcpp::NumericVector read_starts, Rcpp::NumericVector read_length)
{
    Subset_index index(0, length);
    test_read_write_functions_internal<double>(
        REALSXP, length, index,
        write_starts, write_length,
        read_starts, read_length);
}

// [[Rcpp::export]]
void C_test_read_write_functions_with_coercion_subset(
    Rcpp::NumericVector lengths,
    Rcpp::NumericVector write_starts, Rcpp::NumericVector write_length,
    Rcpp::NumericVector read_starts, Rcpp::NumericVector read_length)
{
    size_t total_length = 0;
    Subset_index index;
    for (size_t i = 0; i < (size_t)lengths.size(); i++)
    {
        size_t start = lengths[i];
        size_t stride = i + 2;
        index.push_back(0, lengths[i], i + 2);
        total_length = std::max(total_length, start + (size_t)lengths[i] * stride);
    }
    test_read_write_functions_internal<double>(
        REALSXP, total_length, index,
        write_starts, write_length,
        read_starts, read_length);
}

/*
=========================================================================================
                          unit test for altrep write
=========================================================================================
*/
// [[Rcpp::export]]
void C_set_int_value(SEXP x, size_t i, double v)
{
    if (TYPEOF(x) != INTSXP)
    {
        Rf_error("The variable is not of int type!\n");
    }
    ((int *)DATAPTR(x))[i - 1] = v;
}



/*
=========================================================================================
                          unit test for duplication
=========================================================================================
*/
// [[Rcpp::export]]
void C_test_simple_duplication()
{
    size_t n = 1024 * 1024;
    Protect_guard guard;
    SEXP x = guard.protect(make_int_sequence_altrep(n));
    SEXP y = guard.protect(Rf_duplicate(x));
    throw_if_not(ALTREP(x));
    throw_if_not(ALTREP(y));
    throw_if_not(memcmp(DATAPTR(x), DATAPTR(y), n * sizeof(int))==0);
}
#include <random>
// [[Rcpp::export]]
void C_test_duplication_with_changes()
{
    size_t n = 1024 * 2;
    size_t n_change = 1024;
    Protect_guard guard;
    SEXP x = guard.protect(make_int_sequence_altrep(n));
    int* x_ptr = (int*)DATAPTR(x);
    Rcpp::IntegerVector pool = Rcpp::seq(0, n -1);
    std::random_device rng;
    std::mt19937 urng(rng());
    std::shuffle(pool.begin(), pool.end(), urng);
    for(size_t i=0;i<n_change;i++){
        x_ptr[pool[i]]=x_ptr[pool[i]]+1;
    }
    SEXP y = guard.protect(Rf_duplicate(x));
    int* y_ptr = (int*)DATAPTR(y);
    throw_if_not(ALTREP(x));
    throw_if_not(ALTREP(y));
    throw_if(memcmp(x_ptr,y_ptr,n*sizeof(int))!=0);

    //Change the value back
    for(size_t i=0;i<n_change;i++){
        x_ptr[pool[i]]=x_ptr[pool[i]]-1;
    }
    //return Rcpp::List::create(x,y);
    for(size_t i=0;i<n;i++){
        if(i<n_change){
            throw_if(x_ptr[pool[i]]==y_ptr[pool[i]]);
        }else{
            throw_if(x_ptr[pool[i]]!=y_ptr[pool[i]]);
        }
    }
}



/*
=========================================================================================
                     Unidentified code
=========================================================================================
*/


// [[Rcpp::export]]
void C_reset_int(SEXP x)
{
    if (TYPEOF(x) != INTSXP)
    {
        Rf_error("The variable is not of int type!\n");
    }
    int *ptr = (int *)DATAPTR(x);
    R_xlen_t len = XLENGTH(x);
    for (R_xlen_t i = 0; i < len; i++)
    {
        ptr[i] = 0;
    }
}

// [[Rcpp::export]]
SEXP C_duplicate(SEXP x)
{
    return Rf_duplicate(x);
}

#include "utils.h"

size_t fake_allzero_read(const Travel_altrep_info *altrep_info, void *buffer, size_t offset, size_t length)
{
    filesystem_log("%llu,%llu\n", offset, length);
    memset(buffer, 0, length * get_type_size(altrep_info->type));
    return length;
}
// [[Rcpp::export]]
SEXP C_allzero(size_t n)
{
    Travel_altrep_info altrep_info = {};
    altrep_info.length = n;
    altrep_info.type = REALSXP;
    altrep_info.operations.get_region = fake_allzero_read;
    return Travel_make_altrep(altrep_info);
}

struct RLE
{
    std::vector<double> accumulate_length;
    std::vector<double> value;
};
size_t fake_rle_read(const Travel_altrep_info *altrep_info, void *buffer, size_t offset, size_t length)
{
    RLE *rle = (RLE *)altrep_info->private_data;
    double *ptr = (double *)buffer;
    std::vector<double>::iterator low;
    low = std::upper_bound(rle->accumulate_length.begin(), rle->accumulate_length.end(), offset);
    size_t idx = low - rle->accumulate_length.begin() - 1;
    for (size_t i = 0; i < length; i++)
    {
        while (rle->accumulate_length[idx + 1] <= offset + i)
        {
            idx++;
        }
        ptr[i] = rle->value[idx];
    }
    return length;
}
// [[Rcpp::export]]
SEXP C_RLE(std::vector<double> length, std::vector<double> value)
{
    std::vector<double> accumulate_length;
    accumulate_length.push_back(0);
    size_t total = 0;
    for (size_t i = 0; i < length.size(); i++)
    {
        total += length.at(i);
        accumulate_length.push_back(total);
    }
    RLE *rle = new RLE;
    rle->accumulate_length = accumulate_length;
    rle->value = value;
    Travel_altrep_info altrep_info = {};
    altrep_info.length = accumulate_length.at(accumulate_length.size() - 1);
    altrep_info.type = REALSXP;
    altrep_info.operations.get_region = fake_rle_read;
    altrep_info.private_data = rle;
    altrep_info.protected_data = PROTECT(Travel_shared_ptr<RLE>(rle));
    SEXP x = Travel_make_altrep(altrep_info);
    Rf_unprotect(1);
    return x;
}
