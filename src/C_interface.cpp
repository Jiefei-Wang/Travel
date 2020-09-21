#include "altrep_operations.h"
#include "utils.h"
#include "filesystem_manager.h"

size_t read_altrep(filesystem_file_data &file_data, void *buffer, size_t offset, size_t length)
{
    SEXP wrapped_object = (SEXP)file_data.private_data;
    switch (TYPEOF(wrapped_object))
    {
    case INTSXP:
        INTEGER_GET_REGION(wrapped_object, offset, length, (int *)buffer);
        break;
    case LGLSXP:
        LOGICAL_GET_REGION(wrapped_object, offset, length, (int *)buffer);
        break;
    case REALSXP:
        REAL_GET_REGION(wrapped_object, offset, length, (double *)buffer);
        break;
    default:
        return 0;
    }

    return length;
}

//make_altptr(int type, void *data, size_t length, unsigned int unit_size, file_data_func read_func, SEXP protect)
//[[Rcpp::export]]
SEXP C_make_altPtr_from_altrep(SEXP x)
{
    SEXP altptr_object = make_altptr(TYPEOF(x), x, XLENGTH(x), get_type_size(TYPEOF(x)),read_altrep, x);
    return altptr_object;
}



// [[Rcpp::export]]
SEXP C_getAltData1(SEXP x)
{
    return R_altrep_data1(x);
}
// [[Rcpp::export]]
SEXP C_getAltData2(SEXP x)
{
    return R_altrep_data2(x);
}

// [[Rcpp::export]]
void print_value(SEXP x)
{
    void *ptr = DATAPTR(x);
    if (ptr == NULL || ptr == nullptr)
    {
        Rf_error("The pointer is NULL!\n");
    }
    int max_print = (XLENGTH(x)>100?100:XLENGTH(x));
    for (int i = 0; i < max_print; i++)
    {
        switch (TYPEOF(x))
        {
        case INTSXP:
            Rprintf("%d,", ((int *)ptr)[i]);
            break;
        case REALSXP:
            Rprintf("%f,", ((double *)ptr)[i]);
            break;
        default:
            break;
        }
    }
    Rprintf("\n");
}


// [[Rcpp::export]]
SEXP C_get_ptr(SEXP x){
    return R_MakeExternalPtr(DATAPTR(x),R_NilValue,R_NilValue);
}





size_t fake_read(filesystem_file_data &file_data, void *buffer, size_t offset, size_t length){
    std::string data = "fake read data\n";
    for(size_t i =0;i<length;i++){
        ((char*)buffer)[i+offset] = data.c_str()[(i+offset)%(data.length())];
    }
    return length;
}

//[[Rcpp::export]]
void C_make_fake_file(size_t size)
{
    filesystem_file_data file_data(fake_read,nullptr,size,1);
    add_virtual_file(file_data);
}






/*
//C_make_altPtr_internal(int type, void *data, size_t size, file_data_func read_func, unsigned int unit_size);
struct mydata{
    size_t from;
    size_t by;
};

size_t my_read_func(filesystem_file_data &file_data, void *buffer, size_t offset, size_t length){
    mydata* wrapped_object = (mydata*)file_data.private_data;
    double* ptr = (double*) buffer;
    for(size_t i =0;i<length;i++){
        ptr[i] = wrapped_object->from + wrapped_object->by * (i + offset);
    }
    return length;
}


//[[Rcpp::export]]
SEXP make_seq(size_t from, size_t by, size_t length){
    mydata* data = new mydata;
    data->by = by;
    data->from = from;
    return C_make_altPtr(REALSXP, (void* )data, 8*length, my_read_func, 8);
}


size_t read_SEXP(filesystem_file_data &file_data, void *buffer, size_t offset, size_t length)
{
    filesystem_log("Called, offset:%llu, length:%llu\n",offset, length);
    SEXP wrapped_object = (SEXP)file_data.private_data;
    SEXP data = VECTOR_ELT(wrapped_object, 0);
    Rcpp::Function func = VECTOR_ELT(wrapped_object, 1);
    SEXP result = PROTECT(func(data, Rcpp::wrap(offset + 1), Rcpp::wrap(length)));
    switch (TYPEOF(result))
    {
    case INTSXP:
        INTEGER_GET_REGION(result, 0, length, (int*)buffer);
        break;
    case REALSXP:
        REAL_GET_REGION(result, 0, length, (double*)buffer);
        break;
    default:
        UNPROTECT(1);
        return 0;
    }
    return length;
}
//[[Rcpp::export]]
SEXP C_make_altPtr_from_scratch(int type, SEXP data, size_t length, SEXP read_func, unsigned int unit_size)
{
    Rcpp::List wrapped_object = Rcpp::List::create(data, read_func);
    SEXP altptr_object = C_make_altPtr_internal(type, wrapped_object, length * unit_size, read_SEXP, get_type_size(type));
    if (altptr_object != R_NilValue)
    {
        SET_WRAPPED_DATA(altptr_object, wrapped_object);
    }
    return altptr_object;
}

*/