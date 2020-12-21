get_OS <- function(){
    sysinf <- Sys.info()
    if (!is.null(sysinf)){
        os <- sysinf['sysname']
        if (os == 'Darwin')
            os <- "osx"
    } else { ## mystery machine
        os <- .Platform$OS.type
        if (grepl("^darwin", R.version$os))
            os <- "osx"
        if (grepl("linux-gnu", R.version$os))
            os <- "linux"
    }
    tolower(os)
}


set_verbose<- function(x){
    stopifnot(!is.logical(x))
    C_set_debug_print(x)
    C_set_altrep_print(x)
    C_set_filesystem_print(x)
    C_set_filesystem_log(x)
    if(x){
        if(C_get_print_location()==""){
            C_set_print_location(getwd())
        }
        initial_filesystem_log()
    }else{
        close_filesystem_log()
    }
}
