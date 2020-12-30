#' Create a new ALTREP object from an R ALTREP
#' 
#' Create a new ALTREP object from an R ALTREP, the new ALTREP object
#' owns a data pointer. However, it is a dummy pointer in the sense
#' that the data is not in the memory. The pointer can be operated as
#' a normal pointer but the request of accessing the data will be send to the 
#' wrapped ALTREP object.
#' 
#' @param x an ALTREP object
#' @return A new ALTREP object
#' @examples
#' x <- 1:10
#' y <- wrap_altrep(x)
#' @export
wrap_altrep<-function(x){
    C_wrap_altrep(x)
}

#' Filesystem management
#' 
#' These functions manage the filesystem which Travel is used to
#' create virtual pointers. You should not call these functions
#' manually for Travel will automatically mount the filesystem
#' if it needs one. 
#' 
#' @return Character vector or no return value
#' @examples  
#' ## Automatically determine the mountpoint
#' ## and deploy the filesystem
#' deploy_filesystem()
#' 
#' ## Get mountpoint for the current filesystem
#' get_mountpoint()
#' @rdname filesystem
#' @export
deploy_filesystem <- function(){
    if(is_filesystem_running()){
        return("The filesystem has been running")
    }
    mountpoint <- get_temp_mountpoint()
    set_mountpoint(mountpoint)
    run_filesystem()
}

#' @rdname filesystem
#' @export
get_mountpoint<- function(){
    C_get_mountpoint()
}

#' @rdname filesystem
#' @export
set_mountpoint <- function(path){
    if(is_filesystem_running()){
        stop("The filesystem is running, you cannot change the mountpoint")
    }
    C_set_mountpoint(validate_path(path))
}

#' @rdname filesystem
#' @export
run_filesystem <- function(){
    validate_path(get_mountpoint())
    C_run_filesystem_thread()
}

#' @rdname filesystem
#' @export
stop_filesystem <- function(){
    if(is_filesystem_running()){
        C_stop_filesystem_thread()
    }
}

#' @rdname filesystem
#' @export
is_filesystem_running<-function(){
    C_is_filesystem_running()
}