#' Find path of the Travel header file
#'
#' This function will return the path of the Travel header or
#' the flags that are used to compile C++ code
#' for the developers who want to use C++ level implementation of the
#' `Travel` package
#'
#' @param x Character, "PKG_LIBS" or "PKG_CPPFLAGS"
#' @return path to the header or compiler flags
#' @examples
#' Travel:::pkgconfig("PKG_LIBS")
#' Travel:::pkgconfig("PKG_CPPFLAGS")
#' @export
pkgconfig <- function(x = c("PKG_LIBS", "PKG_CPPFLAGS")){
    x <- match.arg(x)
    pkg_name <- "Travel"
    if(get_OS_bit() == 64){
        folder <- "libs/x64"
    }else{
        folder <- "libs/i386"
    }
    if(x == "PKG_LIBS"){
        folder <- system.file(folder,
                              package = pkg_name, mustWork = FALSE)
        ## Ubuntu will put its compiled library under the folder "libs/" 
        if(folder == ""){
            folder <- system.file("libs",
                                  package = pkg_name, mustWork = TRUE)
        }
        files <- list.files(folder)
        if(length(files)>1){
            ind <- max(which(endsWith(files,".so")),0)
            if(ind == 0){
                ind <- max(which(endsWith(files,".dll")),0)
                stopifnot(ind != 0)
            }
            files <- files[ind]
        }
        result <- paste0("\"",folder,"/",files,"\"")
    }else{
        result <- ""
    }
    cat(result)
}