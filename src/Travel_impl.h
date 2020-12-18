

inline SEXP Travel_make_altrep(Travel_altrep_info altrep_info)
{
    int error_code;
    //Find package namespace
    SEXP pkg_name = Rf_protect(Rf_mkString("Travel"));
    SEXP pkg_space = R_FindNamespace(pkg_name);
    //Load package
    //SEXP call_deploy_filesystem = Rf_protect(Rf_lang1(
    //    Rf_protect(Rf_install("deploy_filesystem"))));
    //R_tryEval(call_deploy_filesystem, pkg_space, &error_code);

    //Pass argument to the package function
    SEXP arg = Rf_protect(R_MakeExternalPtr(&altrep_info, R_NilValue, R_NilValue));
    SEXP call = Rf_protect(Rf_lang2(
        Rf_protect(Rf_install("C_call_Travel_make_altmmap")), arg));
    // Execute the function
    SEXP x = Rf_protect(R_tryEval(call, pkg_space, &error_code));
    Rf_unprotect(5);
    return x;
}
