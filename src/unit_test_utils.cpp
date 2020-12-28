#include "Rcpp.h"
#include "Travel.h"
#include "class_Filesystem_file_data.h"
#include "filesystem_manager.h"
#include "package_settings.h"
size_t read_int_sequence_region(const Travel_altrep_info *altrep_info, void *buffer, size_t offset, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        ((int *)buffer)[i] = offset + i;
    }
    return length;
}
size_t read_int_sequence_block(const Travel_altrep_info *altrep_info, void *buffer,
                                          size_t offset, size_t length, size_t stride)
{
    for (size_t i = 0; i < length; i++)
    {
        ((int *)buffer)[i] = offset + i * stride;
    }
    return length;
}

Filesystem_file_data &make_int_sequence_file(int type, Subset_index index)
{
    Travel_altrep_info altrep_info;
    altrep_info.type = INTSXP;
    altrep_info.length = 1024 * 1024 * 1024;
    altrep_info.operations.get_region = read_int_sequence_region;
    altrep_info.operations.read_blocks = read_int_sequence_block;
    Filesystem_file_identifier file_info = add_filesystem_file(type, index, altrep_info);
    Filesystem_file_data &file_data = get_filesystem_file_data(file_info.file_inode);
    return file_data;
}


SEXP make_int_sequence_altrep(double n)
{
    Travel_altrep_info altrep_info;
    altrep_info.length = n;
    altrep_info.type = INTSXP;
    altrep_info.operations.get_region = read_int_sequence_region;
    altrep_info.operations.read_blocks = read_int_sequence_block;
    SEXP x = Travel_make_altrep(altrep_info);
    return x;
}

SEXP serialize_sequence_func(const Travel_altrep_info* altrep_info){
    return Rf_ScalarReal(altrep_info->length);
}

SEXP C_make_int_sequence_altrep_with_serialize(double n);
// [[Rcpp::export]]
SEXP unserialize_sequence_func(SEXP x){
    return C_make_int_sequence_altrep_with_serialize(Rf_asReal(x));
}
// [[Rcpp::export]]
SEXP C_make_int_sequence_altrep_with_serialize(double n)
{
    Travel_altrep_info altrep_info;
    altrep_info.length = n;
    altrep_info.type = INTSXP;
    altrep_info.operations.get_region = read_int_sequence_region;
    altrep_info.operations.read_blocks = read_int_sequence_block;
    altrep_info.operations.serialize = serialize_sequence_func;
    SEXP pkg_name = Rf_protect(Rf_mkString(PACKAGE_NAME));
    Rcpp::Environment env = R_FindNamespace(pkg_name);
    Rf_unprotect(1);
    altrep_info.operations.unserialize= env["unserialize_sequence_func"];
    SEXP x = Travel_make_altrep(altrep_info);
    return x;
}
