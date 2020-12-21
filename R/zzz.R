#' @useDynLib Travel, .registration = TRUE
#' @importFrom Rcpp sourceCpp
NULL

pkg_data <- new.env()
onExit <-function(e){
    stop_filesystem()
}

.onLoad<- function(libname, pkgname){
    if(Sys.getenv("DEBUG_TRAVEL_PACKAGE")!="T"){
        set_verbose(FALSE)
        #deploy_filesystem()
    }else{
        C_set_debug_print(TRUE)
        C_set_altrep_print(FALSE)
        C_set_filesystem_print(TRUE)
        C_set_filesystem_log(TRUE)
        C_set_print_location(getwd())
    }
    reg.finalizer(pkg_data, onExit, onexit = TRUE)
}

.onUnload<- function(libname, pkgname){
    close_filesystem_log()
    stop_filesystem()
}



