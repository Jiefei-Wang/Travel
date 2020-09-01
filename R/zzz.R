#' @useDynLib AltPtr, .registration = TRUE
#' @importFrom Rcpp sourceCpp
NULL

.onLoad<- function(libname, pkgname){
    set_print_location(getwd())
}