#ifndef HEADER_TRAVEL_PACKAGE
#define HEADER_TRAVEL_PACKAGE

#ifndef Rcpp_hpp
#include <Rinternals.h>
#endif
#include "Travel_package_types.h"
#include "Travel_impl.h"

/*
Create an ALTREP object given the data reading function. There
will be a pointer associated with the ALTREP object. 
Calling R's DATAPTR on the return value is possible without 
allocating the entire data in the memory. 

`altrep_info` is a struct defining the behavior of an altrep object,
its document can be found at the header "Travel_package_types.h".
*/
SEXP Travel_make_altrep(Travel_altrep_info altrep_info);
/*
Get file info from the ALTREP object returned by Travel_make_altrep
*/
SEXP get_file_name(SEXP x);
SEXP get_file_path(SEXP x);


/*
==========A smart external pointer implementation===========
The deleter will be called when the external pointer is garbage collected.
Usage:
    SEXP extPtr = Travel_shared_ptr<int>(new int);
    SEXP extPtrArray = Travel_shared_ptr<int[]>(new int[10]);
=============================================================
*/
template <typename T>
struct Travel_ptr_deleter_impl
{
    static void _(SEXP extptr)
    {
        T *ptr = (T *)R_ExternalPtrAddr(extptr);
        delete ptr;
    }
};
template <typename T>
struct Travel_ptr_deleter_impl<T[]>
{
    static void _(SEXP extptr)
    {
        T *ptr = (T *)R_ExternalPtrAddr(extptr);
        delete[] ptr;
    }
};

template <typename T>
SEXP Travel_shared_ptr(T ptr, SEXP tag = R_NilValue, SEXP prot = R_NilValue)
{
    SEXP extptr = Rf_protect(R_MakeExternalPtr(ptr, tag, prot));
    R_RegisterCFinalizerEx(extptr, Travel_ptr_deleter_impl<T>::_, TRUE);
    Rf_unprotect(1);
    return extptr;
}

template <typename T>
SEXP Travel_shared_ptr(T *ptr, SEXP tag = R_NilValue, SEXP prot = R_NilValue)
{
    SEXP extptr = Rf_protect(R_MakeExternalPtr(ptr, tag, prot));
    R_RegisterCFinalizerEx(extptr, Travel_ptr_deleter_impl<T>::_, TRUE);
    Rf_unprotect(1);
    return extptr;
}

#endif