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
    if(is_filesystem_running()){
        break
    }
    C_make_altPtr_from_altrep(x)
}