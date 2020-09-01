#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "Rcpp.h"
#include "R_ext/Altrep.h"
#include "utils.h"
#include "package_settings.h"

#define NAME_SLOT 0
#define PTR_SLOT 1
#define SIZE_SLOT 2

#define GET_DATA(x) R_altrep_data1(x)
#define GET_PROPS(x) R_altrep_data2(x)

#define GET_SLOT(x, name) VECTOR_ELT(x, name)
#define SET_SLOT(x, name, value) SET_VECTOR_ELT(x, name, value)

#define GET_ALT_SLOT(x, name) GET_SLOT(GET_PROPS(x), name)
#define SET_ALT_SLOT(x, name, value) SET_SLOT(GET_PROPS(x), name, value)

Rboolean altPtr_Inspect(SEXP x, int pre, int deep, int pvec,
                        void (*inspect_subtree)(SEXP, int, int, int))
{
    Rprintf(" (len=%llu, type=%d\n", Rf_xlength(x), TYPEOF(x));
    return TRUE;
}

R_xlen_t altPtr_length(SEXP x)
{
    R_xlen_t size = Rf_xlength(GET_DATA(x));
    return size;
}

void *altPtr_dataptr(SEXP x, Rboolean writeable)
{
    DEBUG_ALTREP(Rprintf("accessing data pointer\n"));
    if (GET_ALT_SLOT(x, PTR_SLOT) == R_NilValue)
    {
        const char* file_name = CHAR(Rf_asChar(GET_ALT_SLOT(x,NAME_SLOT)));
        std::string path(get_mountpoint() + "/" + file_name);
        int fd = open(path.c_str(), O_RDONLY); //read only
        if (fd == -1)
        {
            Rf_error("Fail to open the file %s\n", file_name);
        }
        int *addr = (int *)mmap(NULL, Rf_asReal(GET_SLOT(x, SIZE_SLOT)), PROT_READ, MAP_SHARED, fd, 0);
        if (addr == (void *)-1){
            Rf_error("Fail to map the file %s\n", file_name);
            }
        close(fd);
        //munmap(addr, buf.st_size);
        SEXP ptr = PROTECT(R_MakeExternalPtr(addr,R_NilValue,R_NilValue));
        SET_ALT_SLOT(x, PTR_SLOT,ptr);
        UNPROTECT(1);
    }
    return R_ExternalPtrAddr(GET_ALT_SLOT(x, PTR_SLOT));
}
const void *altPtr_dataptr_or_null(SEXP x)
{
    DEBUG_ALTREP(Rprintf("accessing data pointer or null\n"));
    return altPtr_dataptr(x, Rboolean::TRUE);
}

int altPtr_INTEGER_ELT(SEXP x, R_xlen_t i){
    INTEGER_ELT(GET_DATA(x),i);
}
int altPtr_LOGICAL_ELT(SEXP x, R_xlen_t i){
    LOGICAL_ELT(GET_DATA(x),i);
}
double altPtr_REAL_ELT(SEXP x, R_xlen_t i){
    REAL_ELT(GET_DATA(x),i);
}

R_xlen_t altPtr_INTEGER_REGION(SEXP x, R_xlen_t start, R_xlen_t size, int *out)
{
    DEBUG_ALTREP(Rprintf("accessing numeric region\n"));
    return INTEGER_GET_REGION(x,start,size,out);
}
R_xlen_t altPtr_LOGICAL_REGION(SEXP x, R_xlen_t start, R_xlen_t size, int *out)
{
    DEBUG_ALTREP(Rprintf("accessing numeric region\n"));
    return LOGICAL_GET_REGION(x,start,size,out);
}
R_xlen_t altPtr_REAL_REGION(SEXP x, R_xlen_t start, R_xlen_t size, double *out)
{
    DEBUG_ALTREP(Rprintf("accessing numeric region\n"));
    return REAL_GET_REGION(x,start,size,out);
}



R_altrep_class_t altPtr_real_class;
R_altrep_class_t altPtr_integer_class;
R_altrep_class_t altPtr_logical_class;
//R_altrep_class_t altPtr_raw_class;
//R_altrep_class_t altPtr_complex_class;

R_altrep_class_t getAltClass(int type) {
	switch (type) {
	case REALSXP:
		return altPtr_real_class;
	case INTSXP:
		return altPtr_integer_class;
	case LGLSXP:
		return altPtr_logical_class;
	case RAWSXP:
		//return altPtr_raw_class;
	case CPLXSXP:
		//return altPtr_complex_class;
	case STRSXP:
		//return altPtr_str_class;
	default: Rf_error("Type of %d is not supported yet", type);
	}
	// Just for suppressing the annoying warning, it should never be excuted
	return altPtr_real_class;
}


/*
Register ALTREP class
*/

#define ALT_COMMOM_REGISTRATION(ALT_CLASS, ALT_TYPE, FUNC_PREFIX)\
	ALT_CLASS =R_make_##ALT_TYPE##_class(class_name, PACKAGE_NAME, dll);\
	/* common ALTREP methods */\
	R_set_altrep_Inspect_method(ALT_CLASS, altPtr_Inspect);\
	R_set_altrep_Length_method(ALT_CLASS, altPtr_length);\
	/* ALTVEC methods */\
	R_set_altvec_Dataptr_method(ALT_CLASS, altPtr_dataptr);\
	R_set_altvec_Dataptr_or_null_method(ALT_CLASS, altPtr_dataptr_or_null);\
	R_set_##ALT_TYPE##_Elt_method(ALT_CLASS, altPtr_##FUNC_PREFIX##_ELT);\
	R_set_##ALT_TYPE##_Get_region_method(ALT_CLASS, altPtr_##FUNC_PREFIX##_REGION);


//[[Rcpp::init]]
void init_logical_class(DllInfo* dll)
{
	char class_name[] = "altPtr_logical";
	ALT_COMMOM_REGISTRATION(altPtr_logical_class, altlogical, LOGICAL)
}
//[[Rcpp::init]]
void init_real_class(DllInfo* dll)
{
	char class_name[] = "altPtr_integer";
	ALT_COMMOM_REGISTRATION(altPtr_integer_class, altinteger, INTEGER)
}

//[[Rcpp::init]]
void init_real_class(DllInfo* dll)
{
	char class_name[] = "shared_real";
	ALT_COMMOM_REGISTRATION(altPtr_real_class, altreal, REAL)
}

//[[Rcpp::export]]
Rcpp::List make_altPtr(SEXP x, Rcpp::List options){
    Rcpp::List altPtr_options;
    if(options["name"] != R_NilValue){
        SET_SLOT(altPtr_options, NAME_SLOT, options["name"]);
    }
    SET_SLOT(altPtr_options, PTR_SLOT, R_NilValue);
    SET_SLOT(altPtr_options, SIZE_SLOT, Rf_ScalarReal(get_object_size(x)));
    return altPtr_options;
}

