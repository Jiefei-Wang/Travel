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
pkgconfig <- function(x = c("PKG_LIBS", "PKG_CPPFLAGS")){
    x <- match.arg(x)
    if(x == "PKG_LIBS"){
        folder <- sprintf("libs/%s", .Platform$r_arch)
        folder <- system.file(folder,
                              package = "Travel", mustWork = FALSE)
        if(folder == ""){
            folder <- system.file("libs",
                                  package = "Travel", mustWork = TRUE)
        }
        if(get_OS()=="windows"){
            files <- "Travel.dll"
            filesystem_libs <- paste0('-L"',Sys.getenv("DokanLibrary1"),'lib" -ldokan1')
        }else{
            files <- "Travel.so"
            filesystem_libs <- system("pkg-config fuse --libs",intern=TRUE)
        }
        travel_libs <- paste0('"',folder,"/",files,'"')
        result <- paste0(travel_libs," ",filesystem_libs)
    }else{
        if(get_OS()=="windows"){
            result <- ""
        }else{
            result <- system("pkg-config fuse --cflags",intern=TRUE)
        }
    }
    cat(result)
    invisible(result)
}