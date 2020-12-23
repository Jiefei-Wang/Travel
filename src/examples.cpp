#include "Travel.h"
size_t arithmetic_sequence_region(const Travel_altrep_info *altrep_info, void *buffer,
                                  size_t offset, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        ((double *)buffer)[i] = offset + i + 1;
    }
    return length;
}

// [[Rcpp::export]]
SEXP Travel_compact_seq(size_t n)
{
    Travel_altrep_info altrep_info;
    altrep_info.length = n;
    altrep_info.type = REALSXP;
    altrep_info.operations.get_region = arithmetic_sequence_region;
    SEXP x = Travel_make_altrep(altrep_info);
    return x;
}



