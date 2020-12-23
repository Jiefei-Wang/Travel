#include "class_Protect_guard.h"
Protect_guard::Protect_guard():protect_num(0){}
Protect_guard::~Protect_guard()
{
    if (protect_num != 0)
        UNPROTECT(protect_num);
}
SEXP Protect_guard::protect(SEXP x)
{
    protect_num++;
    return PROTECT(x);
}