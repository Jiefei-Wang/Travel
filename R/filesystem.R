validate_path <-function(path){
    path <- normalizePath(path, mustWork = FALSE, winslash = "/")
    is_driver_path <- regexpr("^[a-zA-Z]:/$",path)==1
    path <- normalizePath(path, mustWork = FALSE)
    if(dir.exists(path)){
        if(is_driver_path){
            stop("The driver path <", path,
                 "> is in used and cannot be used as a mount point")
        }
        files <- list.files(path, all.files = TRUE, recursive = TRUE)
        if(length(files)!=0){
            stop("The directory <",path,"> is not empty!")
        }
        path <- normalizePath(path, mustWork = TRUE)
    }else{
        ## Avoid the driver path(e.g. "C:/")
        if(!is_driver_path){
            dir.create(path, recursive = TRUE)
            path <- normalizePath(path, mustWork = TRUE)
        }
    }
    path
}


get_mountpoint<- function(){
    C_get_mountpoint()
}
set_mountpoint <- function(path){
    if(is_filesystem_running()){
        stop("The filesystem is running, you cannot change the mountpoint")
    }
    C_set_mountpoint(validate_path(path))
}
run_filesystem <- function(){
    validate_path(get_mountpoint())
    C_run_filesystem_thread()
}
stop_filesystem <- function(){
    if(is_filesystem_running()){
        C_stop_filesystem_thread()
        if(!is_filesystem_running()){
            unlink(get_mountpoint(),recursive = TRUE)
        }
    }
}
is_filesystem_running<-function(){
    C_is_filesystem_running()
}
