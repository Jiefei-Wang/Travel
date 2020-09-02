#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
//function close
#include <unistd.h>

#include "Rcpp.h"
#include "R_ext/Altrep.h"
#include "utils.h"
#include "package_settings.h"
#include "fuse.h"
#include "altrep_operations.h"

#define SLOT_NUM 3
#define NAME_SLOT 0
#define PTR_SLOT 1
#define SIZE_SLOT 2

#define GET_PROPS(x) ((Rcpp::List)R_altrep_data2(x))

#define GET_NAME(x) (x[NAME_SLOT])
#define GET_PTR(x) (x[PTR_SLOT])
#define GET_SIZE(x) (x[SIZE_SLOT])

#define GET_ALT_NAME(x) (GET_PROPS(x)[NAME_SLOT])
#define GET_ALT_PTR(x) (GET_PROPS(x)[PTR_SLOT])
#define GET_ALT_SIZE(x) (GET_PROPS(x)[SIZE_SLOT])

Rboolean altPtr_Inspect(SEXP x, int pre, int deep, int pvec,
                        void (*inspect_subtree)(SEXP, int, int, int))
{
    Rprintf("altrep pointer object(len=%llu, type=%d)\n", Rf_xlength(x), TYPEOF(x));
    return TRUE;
}

R_xlen_t altPtr_length(SEXP x)
{
    R_xlen_t size = Rf_xlength(GET_ALT_DATA(x));
    DEBUG_ALTREP(Rprintf("accessing length: %d\n", size));
    return size;
}

void *altPtr_dataptr(SEXP x, Rboolean writeable)
{
    DEBUG_ALTREP(Rprintf("accessing data pointer\n"));
    SEXP ptr = GET_ALT_PTR(x);
    if(ptr==R_NilValue){
        return NULL;
    }else{
        return R_ExternalPtrAddr(ptr);
    }
}
const void *altPtr_dataptr_or_null(SEXP x)
{
    DEBUG_ALTREP(Rprintf("accessing data pointer or null\n"));
    return altPtr_dataptr(x, Rboolean::TRUE);
    //return NULL;
}

int altPtr_INTEGER_ELT(SEXP x, R_xlen_t i)
{
    return INTEGER_ELT(GET_ALT_DATA(x), i);
}
int altPtr_LOGICAL_ELT(SEXP x, R_xlen_t i)
{
    return LOGICAL_ELT(GET_ALT_DATA(x), i);
}
double altPtr_REAL_ELT(SEXP x, R_xlen_t i)
{
    return REAL_ELT(GET_ALT_DATA(x), i);
}

R_xlen_t altPtr_INTEGER_REGION(SEXP x, R_xlen_t start, R_xlen_t size, int *out)
{
    DEBUG_ALTREP(Rprintf("accessing numeric region\n"));
    return INTEGER_GET_REGION(GET_ALT_DATA(x), start, size, out);
}
R_xlen_t altPtr_LOGICAL_REGION(SEXP x, R_xlen_t start, R_xlen_t size, int *out)
{
    DEBUG_ALTREP(Rprintf("accessing numeric region\n"));
    return LOGICAL_GET_REGION(GET_ALT_DATA(x), start, size, out);
}
R_xlen_t altPtr_REAL_REGION(SEXP x, R_xlen_t start, R_xlen_t size, double *out)
{
    DEBUG_ALTREP(Rprintf("accessing numeric region\n"));
    return REAL_GET_REGION(GET_ALT_DATA(x), start, size, out);
}

R_altrep_class_t altPtr_real_class;
R_altrep_class_t altPtr_integer_class;
R_altrep_class_t altPtr_logical_class;
//R_altrep_class_t altPtr_raw_class;
//R_altrep_class_t altPtr_complex_class;

R_altrep_class_t getAltClass(int type)
{
    switch (type)
    {
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
    default:
        Rf_error("Type of %d is not supported yet", type);
    }
    // Just for suppressing the annoying warning, it should never be excuted
    return altPtr_real_class;
}

/*
Register ALTREP class
*/

#define ALT_COMMOM_REGISTRATION(ALT_CLASS, ALT_TYPE, FUNC_PREFIX)           \
    ALT_CLASS = R_make_##ALT_TYPE##_class(class_name, PACKAGE_NAME, dll);   \
    /* common ALTREP methods */                                             \
    R_set_altrep_Inspect_method(ALT_CLASS, altPtr_Inspect);                 \
    R_set_altrep_Length_method(ALT_CLASS, altPtr_length);                   \
    /* ALTVEC methods */                                                    \
    R_set_altvec_Dataptr_method(ALT_CLASS, altPtr_dataptr);                 \
    R_set_altvec_Dataptr_or_null_method(ALT_CLASS, altPtr_dataptr_or_null); \
    R_set_##ALT_TYPE##_Elt_method(ALT_CLASS, altPtr_##FUNC_PREFIX##_ELT);   \
    R_set_##ALT_TYPE##_Get_region_method(ALT_CLASS, altPtr_##FUNC_PREFIX##_REGION);

//[[Rcpp::init]]
void init_logical_class(DllInfo *dll)
{
    char class_name[] = "altPtr_logical";
    ALT_COMMOM_REGISTRATION(altPtr_logical_class, altlogical, LOGICAL)
}
//[[Rcpp::init]]
void init_integer_class(DllInfo *dll)
{
    char class_name[] = "altPtr_integer";
    ALT_COMMOM_REGISTRATION(altPtr_integer_class, altinteger, INTEGER)
}

//[[Rcpp::init]]
void init_real_class(DllInfo *dll)
{
    char class_name[] = "shared_real";
    ALT_COMMOM_REGISTRATION(altPtr_real_class, altreal, REAL)
}

static void ptr_finalizer(SEXP extPtr)
{
    Rcpp::List props = R_ExternalPtrTag(extPtr);
    std::string name = Rcpp::as<std::string>(GET_NAME(props));
    uint64_t size = Rcpp::as<uint64_t>(GET_SIZE(props));
    void* ptr = R_ExternalPtrAddr(GET_PTR(props));
    DEBUG_ALTPTR(Rprintf("Finalizer, name:%s, size:%llu\n", name.c_str(), size));
    munmap(ptr, size);
    remove_altrep_from_fuse(GET_NAME(props));
}


/*
name can be NULL
*/
//[[Rcpp::export]]
SEXP C_make_altPtr(SEXP x, SEXP name)
{
    Rcpp::List altPtr_options(SLOT_NUM);
    GET_SIZE(altPtr_options) = Rf_ScalarReal(get_object_size(x));
    R_altrep_class_t alt_class = getAltClass(TYPEOF(x));
    SEXP result = PROTECT(R_new_altrep(alt_class, x, altPtr_options));
    std::string file_name = add_altrep_to_fuse(result, name);
    GET_NAME(altPtr_options) = file_name;
    sleep(1);
    //Do the file mapping
    std::string file_full_path(get_mountpoint() + "/" + file_name);
    int fd = open(file_full_path.c_str(), O_RDONLY); //read only
    if (fd == -1)
    {
        UNPROTECT(1);
        Rf_error("Fail to open the file %s\n", file_full_path.c_str());
    }
    int *addr = (int *)mmap(NULL, Rf_asReal(GET_SIZE(altPtr_options)), PROT_READ, MAP_SHARED, fd, 0);
    if (addr == (void *)-1)
    {
        UNPROTECT(1);
        Rf_error("Fail to map the file %s\n", file_full_path.c_str());
    }
    close(fd);
    SEXP ptr = PROTECT(R_MakeExternalPtr(addr, altPtr_options, R_NilValue));
    //Register finalizer for the pointer
    R_RegisterCFinalizerEx(ptr, ptr_finalizer, TRUE);
    GET_PTR(altPtr_options) = ptr;

    UNPROTECT(2);
    return result;
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
int get_int_value(SEXP x, int i){
    return ((int*)DATAPTR(x))[i];
}
