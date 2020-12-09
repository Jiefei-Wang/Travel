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
        folder <- sprintf("usrlib/%s", .Platform$r_arch)
        folder <- system.file(folder,
                              package = "Travel", mustWork = FALSE)
        if(folder == ""){
            folder <- system.file("usrlib",
                                  package = "Travel", mustWork = TRUE)
        }
        files <- "Travel.a"
        result <- paste0("\"",folder,"/",files,"\" ",
                         '-L"',Sys.getenv("DokanLibrary1"),'lib" -ldokan1')
    }else{
        result <- ""
    }
    cat(result)
    invisible(result)
}