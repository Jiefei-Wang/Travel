#' @useDynLib Travel, .registration = TRUE
#' @importFrom Rcpp sourceCpp
NULL

.onLoad<- function(libname, pkgname){
    if(Sys.getenv("DEBUG_TRAVEL_PACKAGE")==""){
        deploy_filesystem()
    }else{
        C_set_debug_print(TRUE)
        C_set_altrep_print(FALSE)
        C_set_filesystem_print(TRUE)
        C_set_filesystem_log(TRUE)
        C_set_print_location(getwd())
    }
}

.onUnload<- function(libname, pkgname){
    stop_filesystem()
}