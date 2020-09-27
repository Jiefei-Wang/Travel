#include <Rcpp.h>
#include "Travel.h"
//#include "utils.h"
#include "filesystem_manager.h"
/*
Create an integer vector for testing the altrep functions
*/
size_t fake_integer_read(filesystem_file_data &file_data, void *buffer, size_t offset, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        ((int *)buffer)[i] = (offset + i) % (1024 * 1024) + 1;
    }
    return length;
}
// [[Rcpp::export]]
SEXP C_make_test_integer_altrep(double n)
{
    return Travel_make_altrep(INTSXP, n, fake_integer_read, nullptr, sizeof(int));
}

/*
Create a fake file in the mounted filesystem
*/
size_t fake_read(filesystem_file_data &file_data, void *buffer, size_t offset, size_t length)
{
    std::string data = "fake read data\n";
    for (size_t i = 0; i < length; i++)
    {
        ((char *)buffer)[i] = data.c_str()[(i + offset) % (data.length())];
    }
    return length;
}

//[[Rcpp::export]]
void C_make_fake_file(size_t size)
{
    add_virtual_file(fake_read, nullptr, size);
}

// [[Rcpp::export]]
void C_set_real_value(SEXP x, size_t i, double v)
{
    if (TYPEOF(x) != REALSXP)
    {
        Rf_error("The variable is not of REAL type!\n");
    }
    ((double *)DATAPTR(x))[i - 1] = v;
}
// [[Rcpp::export]]
void C_set_int_value(SEXP x, size_t i, double v)
{
    if (TYPEOF(x) != INTSXP)
    {
        Rf_error("The variable is not of REAL type!\n");
    }
    ((int *)DATAPTR(x))[i - 1] = v;
}