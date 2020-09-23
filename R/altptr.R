
wrap_altrep<-function(x){
    start_time <- Sys.time()
    repeat{
        if(is_filesystem_running()){
            break
        }
        if(Sys.time() - start_time>1){
            break
        }
    }
    C_make_altPtr_from_altrep(x)
}