#include <R_ext/Altrep.h>

#define GET_ALT_DATA(x) R_altrep_data1(x)

R_altrep_class_t getAltClass(int type);
