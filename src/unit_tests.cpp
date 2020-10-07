#include <Rcpp.h>
#include "Travel.h"
//#include "utils.h"
#include "filesystem_manager.h"
/*
Create an integer vector for testing the altrep functions
*/
size_t fake_integer_read(const Travel_altrep_info* altrep_info, void *buffer, size_t offset, size_t length)
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
    Travel_altrep_info altrep_info = {};
    altrep_info.length = n;
    altrep_info.type = INTSXP;
    altrep_info.operations.get_region = fake_integer_read;

    return Travel_make_altptr(altrep_info);
}

/*
Create a fake file in the mounted filesystem
*/
size_t fake_read(const Travel_altrep_info* altrep_info, void *buffer, size_t offset, size_t length)
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
    Travel_altrep_info altrep_info = {};
    altrep_info.type = RAWSXP;
    altrep_info.length = size;
    altrep_info.operations.get_region = fake_read;
    add_virtual_file(RAWSXP,altrep_info);
}

//[[Rcpp::export]]
void C_make_fake_file2(size_t size)
{
    Travel_altrep_info altrep_info = {};
    altrep_info.operations.get_region = fake_integer_read;
    altrep_info.type = RAWSXP;
    altrep_info.length = size;
    add_virtual_file(RAWSXP,altrep_info);
}

SEXP make_altptr_from_file(std::string path, int type, size_t length);
//[[Rcpp::export]]
SEXP C_make_altptr_from_file(SEXP path, SEXP type, size_t length)
{
    std::string path_str = Rcpp::as<std::string>(path);
    std::string type_str = Rcpp::as<std::string>(type);
    int type_num;
    if(type_str=="logical"){
        type_num = LGLSXP;
    }else if(type_str == "integer"){
        type_num = INTSXP;
    }else if(type_str=="double"|| type_str =="real"){
        type_num = REALSXP;
    }else{
        Rf_error("Unknown type <%s>\n", type_str.c_str());
    }
    return make_altptr_from_file(path_str, type_num, length);
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
void C_reset_int(SEXP x){
     if (TYPEOF(x) != INTSXP)
    {
        Rf_error("The variable is not of int type!\n");
    }
    int* ptr = (int*)DATAPTR(x);
    R_xlen_t len = XLENGTH(x);
    for(R_xlen_t i=0;i< len;i++){
        ptr[i]=0;
    }
}


// [[Rcpp::export]]
SEXP C_duplicate(SEXP x){
    return Rf_duplicate(x);
}

// [[Rcpp::export]]
bool C_is_altrep(SEXP x){
    return ALTREP(x);
}