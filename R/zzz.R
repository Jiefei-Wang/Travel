#' @useDynLib Travel, .registration = TRUE
#' @importFrom Rcpp sourceCpp
NULL

pkg_data <- new.env()
pkg_data$pkg_unloaded <- FALSE

onExit <-function(e){
    if(!pkg_data$pkg_unloaded)
        stop_filesystem()
}

.onLoad<- function(libname, pkgname){
    if(Sys.getenv("DEBUG_TRAVEL_PACKAGE")!="T"){
        set_verbose(FALSE)
    }else{
        set_verbose(FALSE)
        C_set_debug_print(TRUE)
        C_set_altrep_print(FALSE)
        C_set_filesystem_print(TRUE)
        set_filesystem_log(TRUE)
    }
    reg.finalizer(pkg_data, onExit, onexit = TRUE)
}

.onUnload<- function(libname, pkgname){
    C_close_filesystem_log()
    stop_filesystem()
    pkg_data$pkg_unloaded <- TRUE
}



