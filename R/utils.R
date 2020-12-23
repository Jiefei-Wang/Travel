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

set_filesystem_log<-function(x){
    if(x){
        if(C_filesystem_log_location()==""){
            C_set_filesystem_log_location(getwd())
        }
    }
    C_set_filesystem_log(x)
}


set_verbose<- function(x){
    stopifnot(is.logical(x))
    C_set_debug_print(x)
    C_set_altrep_print(x)
    C_set_filesystem_print(x)
    #set_filesystem_log(x)
}

copy_header <- function(){
    src_folder <- "src/"
    dest_folder <- "inst/include/Travel/"
    files <- c("Travel.h","Travel_package_types.h","Travel_impl.h")
    for(i in files){
        src_file <- paste0(src_folder,i)
        dest_file <- paste0(dest_folder,i)
        file.copy(src_file,dest_file,overwrite=TRUE)
    }
}
