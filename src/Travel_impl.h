

inline SEXP Travel_make_altrep(Travel_altrep_info altrep_info){
    SEXP arg = Rf_protect(R_MakeExternalPtr(&altrep_info,R_NilValue,R_NilValue));
    SEXP call = Rf_protect(Rf_lang2(Rf_install("C_call_Travel_make_altmmap"), arg));
    SEXP pkg_name = Rf_protect(Rf_mkString("Travel"));
    SEXP pkg_space = R_FindNamespace(pkg_name);
    // Execute the function
    int error_code;
    SEXP x = Rf_protect(R_tryEval(call, pkg_space, &error_code));
    Rf_unprotect(4);
    return x;
}
