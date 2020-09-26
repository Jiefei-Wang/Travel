#ifndef HEADER_TRAVEL_PACKAGE
#define HEADER_TRAVEL_PACKAGE

#include <Rcpp.h>
#include <R_ext/Altrep.h>
#include "Travel_types.h"

SEXP make_altptr(int type, void *data, size_t length, unsigned int unit_size, file_data_func read_func, SEXP protect = R_NilValue);

template<typename T>
void ptr_finalizer(SEXP extptr){
    T ptr = (T)R_ExternalPtrAddr(extptr);
    delete ptr;
}

template<typename T>
void ptr_array_finalizer(SEXP extptr){
    T* ptr = (T*)R_ExternalPtrAddr(extptr);
    delete[] ptr;
}

template<typename T>
struct Travel_shared_ptr_impl{
    static SEXP _(T data, SEXP tag, SEXP prot) { 
        SEXP extptr = Rf_protect(R_MakeExternalPtr(data, tag, prot));
        R_RegisterCFinalizerEx(extptr, ptr_finalizer<T>, TRUE);
        Rf_unprotect(1);
        return extptr;
    }
};
template<typename T>
struct Travel_shared_ptr_impl<T[]>{
    static SEXP _(T* data, SEXP tag, SEXP prot) { 
        SEXP extptr = Rf_protect(R_MakeExternalPtr(data, tag, prot));
        R_RegisterCFinalizerEx(extptr, ptr_array_finalizer<T>, TRUE);
        Rf_unprotect(1);
        return extptr;
    }
};

template<typename T>
SEXP Travel_shared_ptr(T data, SEXP tag = R_NilValue, SEXP prot = R_NilValue)
{
    return Travel_shared_ptr_impl<T>::_(data,tag,prot);
}


#endif