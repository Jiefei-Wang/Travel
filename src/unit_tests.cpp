#include <Rcpp.h>
#include "Travel.h"
//#include "utils.h"
#include "filesystem_manager.h"
#include "altrep_manager.h"
#include "utils.h"

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
void C_test_Subset_index_conversion(){
    Rcpp::NumericVector idx1=Rcpp::NumericVector::create(1,2,3,4,6,7,8,9,11,13,15,17,18);
    Subset_index ref_index1(0,100);
    Subset_index index1 = Subset_index::to_subset_index(idx1,ref_index1);
    throw_if_not(index1.total_length ==(size_t)idx1.length());
    throw_if_not(index1.starts[0]==0);
    throw_if_not(index1.starts[1]==5);
    throw_if_not(index1.starts[2]==10);
    throw_if_not(index1.starts[3]==17);
    throw_if_not(index1.lengths[0]==4);
    throw_if_not(index1.lengths[1]==4);
    throw_if_not(index1.lengths[2]==4);
    throw_if_not(index1.lengths[3]==1);
    throw_if_not(index1.strides[0]==1);
    throw_if_not(index1.strides[1]==1);
    throw_if_not(index1.strides[2]==2);
    throw_if_not(index1.strides[3]==1);
    throw_if_not(index1.partial_lengths[0]==0);
    throw_if_not(index1.partial_lengths[1]==4);
    throw_if_not(index1.partial_lengths[2]==8);
    throw_if_not(index1.partial_lengths[3]==12);
    for(size_t i=0;i<index1.total_length;i++){
        throw_if_not(index1.get_source_index(i)==idx1[i]-1);
    }

    //Unordered subset
    Rcpp::NumericVector idx2=Rcpp::NumericVector::create(6,7,8,9,1,2,3,4);
    Subset_index ref_index2(0,100);
    Subset_index index2 = Subset_index::to_subset_index(idx2,ref_index2);
    throw_if_not(index2.total_length ==(size_t)idx2.length());
    throw_if_not(index2.starts[0]==5);
    throw_if_not(index2.starts[1]==0);
    throw_if_not(index2.lengths[0]==4);
    throw_if_not(index2.lengths[1]==4);
    throw_if_not(index2.strides[0]==1);
    throw_if_not(index2.strides[1]==1);
    throw_if_not(index2.partial_lengths[0]==0);
    throw_if_not(index2.partial_lengths[1]==4);
    for(size_t i=0;i<index2.total_length;i++){
        throw_if_not(index2.get_source_index(i)==idx2[i]-1);
    }
    //subset with reference
    //c(7,8,1,3,4)
    Rcpp::NumericVector idx3=Rcpp::NumericVector::create(2,3,5,7,8);
    Subset_index index3 = Subset_index::to_subset_index(idx3,index2);
    throw_if_not(index3.total_length ==(size_t)idx3.length());
    throw_if_not(index3.starts[0]==6);
    throw_if_not(index3.starts[1]==0);
    throw_if_not(index3.starts[2]==3);
    throw_if_not(index3.lengths[0]==2);
    throw_if_not(index3.lengths[1]==2);
    throw_if_not(index3.lengths[2]==1);
    throw_if_not(index3.strides[0]==1);
    throw_if_not(index3.strides[1]==2);
    throw_if_not(index3.strides[2]==1);
    throw_if_not(index3.partial_lengths[0]==0);
    throw_if_not(index3.partial_lengths[1]==2);
    throw_if_not(index3.partial_lengths[2]==4);
    for(size_t i=0;i<index3.total_length;i++){
        throw_if_not(index3.get_source_index(i)==idx2[idx3[i]-1]-1);
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

    cache1 =cache1;
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
                          unit test for general read/write functions
=========================================================================================
*/
#include "read_write_operations.h"
/*
Create a fake file in the mounted filesystem
*/
size_t read_int_arithmetic_sequence(const Travel_altrep_info *altrep_info, void *buffer, size_t offset, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        ((int *)buffer)[i] = offset + i;
    }
    return length;
}

void test_read_write_functions_internal(
    int type, size_t length, Subset_index index,
    Rcpp::NumericVector write_starts, Rcpp::NumericVector write_length,
    Rcpp::NumericVector read_starts, Rcpp::NumericVector read_length)
{
    Travel_altrep_info altrep_info = {};
    altrep_info.type = INTSXP;
    altrep_info.length = length;
    altrep_info.operations.get_region = read_int_arithmetic_sequence;
    Filesystem_file_identifier file_info = add_filesystem_file(type, index, altrep_info);
    Filesystem_file_data &file_data = get_filesystem_file_data(file_info.file_inode);
    uint8_t &type_size = file_data.unit_size;
    
    throw_if_not(file_data.file_length==index.total_length);
    //Create the test data
    size_t data_length = file_data.file_length;
    std::unique_ptr<char[]> data(new char[type_size * data_length]);
    char *data_ptr = data.get();
    for (size_t i = 0; i < data_length; i++)
    {
        if (type == INTSXP)
        {
            ((int *)data_ptr)[i] = index.get_source_index(i);
        }
        else
        {
            ((double *)data_ptr)[i] = index.get_source_index(i);
        }
    }
    //The data that is written is the negative value of the original value in the file
    for (R_xlen_t i = 0; i < write_starts.length(); i++)
    {
        size_t elt_offset = write_starts[i];
        size_t offset = elt_offset * type_size;
        size_t size = write_length[i] * type_size;
        for (size_t j = 0; j < write_length[i]; j++)
        {
            if (type == INTSXP)
            {
                ((int *)data_ptr)[elt_offset + j] = -index.get_source_index(elt_offset + j);
            }
            else
            {
                ((double *)data_ptr)[elt_offset + j] = -index.get_source_index(elt_offset + j);
            }
        }
        size_t true_write_size = general_write_func(file_data, data_ptr + offset, offset, size);
        if (true_write_size != size)
        {
            Rf_error("The write size does not match, offset: %llu, expected: %llu, true: %llu, file size: %llu",
                     (uint64_t)offset, (uint64_t)size, (uint64_t)true_write_size, (uint64_t)file_data.file_size);
        }
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
        if (true_read_size != size)
        {
            Rf_error("The read size does not match, offset: %llu, expected: %llu, true: %llu, file size: %llu",
                     (uint64_t)offset, (uint64_t)size, (uint64_t)true_read_size, (uint64_t)file_data.file_size);
        }
        for (size_t j = 0; j < read_length[i]; j++)
        {
            double expected_value;
            double true_value;
            if (type == INTSXP)
            {
                expected_value = ((int *)data_ptr)[elt_offset + j];
                true_value = ((int *)ptr)[j];
            }
            else
            {
                expected_value = ((double *)data_ptr)[elt_offset + j];
                true_value = ((double *)ptr)[j];
            }
            if (expected_value != true_value)
            {
                Rf_error("The read data does not match, index: %llu, expected: %f, true: %f",
                         (uint64_t)(elt_offset + j), expected_value, true_value);
            }
        }
    }
    if (file_data.write_cache.size() == 0)
    {
        Rf_error("The write cache seems untouched.");
    }
    remove_filesystem_file(file_info.file_inode);
}

// [[Rcpp::export]]
void C_test_read_write_functions_native(
    size_t length,
    Rcpp::NumericVector write_starts, Rcpp::NumericVector write_length,
    Rcpp::NumericVector read_starts, Rcpp::NumericVector read_length)
{
    Subset_index index(0, length);
    test_read_write_functions_internal(
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
    test_read_write_functions_internal(
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
    Subset_index index;
    for(size_t i=0;i<lengths.size();i++){
        index.push_back(0,lengths[i],i+2);
    }
    test_read_write_functions_internal(
        REALSXP, index.get_source_index(index.total_length-1)+1, index,
        write_starts, write_length,
        read_starts, read_length);
}


/*
=========================================================================================
                     make an arithmetic sequence altrep
=========================================================================================
*/
// [[Rcpp::export]]
SEXP C_make_arithmetic_sequence_altrep(double n)
{
    Travel_altrep_info altrep_info;
    altrep_info.length = n;
    altrep_info.type = INTSXP;
    altrep_info.operations.get_region = read_int_arithmetic_sequence;
    SEXP x = Travel_make_altrep(altrep_info);
    return x;
}



/*
=========================================================================================
                     Unidentified code
=========================================================================================
*/
//[[Rcpp::export]]
SEXP C_make_altmmap_from_file(SEXP path, SEXP type, size_t length)
{
    std::string path_str = Rcpp::as<std::string>(path);
    std::string type_str = Rcpp::as<std::string>(type);
    int type_num;
    if (type_str == "logical")
    {
        type_num = LGLSXP;
    }
    else if (type_str == "integer")
    {
        type_num = INTSXP;
    }
    else if (type_str == "double" || type_str == "real")
    {
        type_num = REALSXP;
    }
    else
    {
        Rf_error("Unknown type <%s>\n", type_str.c_str());
    }
    return make_altmmap_from_file(path_str, type_num, length);
}

// [[Rcpp::export]]
void C_set_real_value(SEXP x, size_t i, double v)
{
    if (TYPEOF(x) != REALSXP)
    {
        Rf_error("The variable is not of double type!\n");
    }
    ((double *)DATAPTR(x))[i - 1] = v;
}
// [[Rcpp::export]]
void C_set_int_value(SEXP x, size_t i, double v)
{
    if (TYPEOF(x) != INTSXP)
    {
        Rf_error("The variable is not of int type!\n");
    }
    ((int *)DATAPTR(x))[i - 1] = v;
}

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

// [[Rcpp::export]]
bool C_is_altrep(SEXP x)
{
    return ALTREP(x);
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