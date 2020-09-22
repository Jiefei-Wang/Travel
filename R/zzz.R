#' @useDynLib Travel, .registration = TRUE
#' @importFrom Rcpp sourceCpp
NULL

.onLoad<- function(libname, pkgname){
    C_set_print_location(getwd())
}

.onUnload<- function(libname, pkgname){
    stop_filesystem()
}