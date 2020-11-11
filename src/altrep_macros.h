#ifndef R_INTERNALS_H_
#include <Rcpp.h>
#endif
#define SLOT_NUM 4
#define NAME_SLOT 0
#define FILE_HANDLE_SLOT 1
#define SIZE_SLOT 2
#define LENGTH_SLOT 3

#define GET_PROTECTED_DATA(x) R_altrep_data1(x)
#define SET_PROTECTED_DATA(x, v) R_set_altrep_data1(x, v)


#define GET_PROPS(x) R_altrep_data2(x)
//These macros assume x is a list of options
#define GET_PROPS_NAME(x) VECTOR_ELT(x,NAME_SLOT)
//File handle external pointer
#define GET_PROPS_EXTPTR(x) VECTOR_ELT(x,FILE_HANDLE_SLOT)
//File handle itself
#define GET_PROPS_FILE_HANDLE(x) ((file_map_handle *)R_ExternalPtrAddr(GET_PROPS_EXTPTR(x)))
#define GET_PROPS_SIZE(x) VECTOR_ELT(x,SIZE_SLOT)
#define GET_PROPS_LENGTH(x) VECTOR_ELT(x,LENGTH_SLOT)

#define SET_PROPS_NAME(x,v) SET_VECTOR_ELT(x,NAME_SLOT,v)
#define SET_PROPS_EXTPTR(x,v) SET_VECTOR_ELT(x,FILE_HANDLE_SLOT,v)
#define SET_PROPS_SIZE(x,v) SET_VECTOR_ELT(x,SIZE_SLOT,v)
#define SET_PROPS_LENGTH(x,v) SET_VECTOR_ELT(x,LENGTH_SLOT,v)


//These macros assume x is an ALTREP object
#define GET_ALT_NAME(x) (GET_PROPS_NAME(GET_PROPS(x)))
#define GET_ALT_HANDLE_EXTPTR(x) (GET_PROPS_EXTPTR(GET_PROPS(x)))
#define GET_ALT_FILE_HANDLE(x) (GET_PROPS_FILE_HANDLE(GET_PROPS(x)))
#define GET_ALT_SIZE(x) (GET_PROPS_SIZE(GET_PROPS(x)))
#define GET_ALT_LENGTH(x) (GET_PROPS_LENGTH(GET_PROPS(x)))






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
