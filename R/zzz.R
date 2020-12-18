#' @useDynLib Travel, .registration = TRUE
#' @importFrom Rcpp sourceCpp
NULL

.onLoad<- function(libname, pkgname){
    if(Sys.getenv("DEBUG_TRAVEL_PACKAGE")!="T"){
        set_verbose(FALSE)
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
    close_filesystem_log()
    stop_filesystem()
}


dummy_env <- new.env()
.onExit <- function(x){
    stop_filesystem()
} 
reg.finalizer(dummy_env, .onExit, onexit = TRUE)

