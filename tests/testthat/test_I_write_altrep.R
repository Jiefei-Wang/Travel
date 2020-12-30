context("Testing filesystem writting feature with altrep")
set_verbose(FALSE)
rm(list=ls())
gc()

## Skip it for linux for there is a know bug
if(get_OS()!="linux"){
    block_size <- 1024*1024
    
    test_that("write to an integer sequece",{
        n <- 1024*1024*16/4
        expect_error(x <- C_make_int_sequence_altrep(n),NA)
        y <- 0:(n-1)
        expect_equal(x,y)
        
        ## Check if the cache number is 0
        expect_true(nrow(C_get_altmmap_cache(x))== 0)
        
        
        ## Change the first value of x, expect creating 1 block
        C_set_int_value(x,1,-10)
        y[1] <- -10
        C_flush_altrep(x)
        expect_true(nrow(C_get_altmmap_cache(x))== 1)
        
        ## Check the value of the file
        ## Check by the variable
        expect_equal(x,y)
        ## Check by the file connection
        file_path <- C_get_file_path(x)
        con <- file(file_path, open = "rb")
        data <- readBin(con, integer(),n)
        expect_equal(x,data)
        close(con)
        
        ## Change the all value of x, expect creating 4096 block
        ind <- sample(1:length(x),length(x))
        j <- 0
        for(i in ind){
            C_set_int_value(x,i,-i)
            y[i] <- -i
            j = j + 1
            if(j>n/5){
                j <- 0
                C_flush_altrep(x)
            }
        }
        C_flush_altrep(x)
        expect_true(nrow(C_get_altmmap_cache(x))== n*4/4096)
        
        
        ## Check the value of the file
        ## Check by the variable
        expect_equal(x,y)
        ## Check by the file connection
        file_path <- C_get_file_path(x)
        con <- file(file_path, open = "rb")
        data <- readBin(con, integer(),n)
        expect_equal(x,data)
        close(con)
    })
    
    gc()
    test_that("Check virtual file number",{
        expect_true(nrow(get_virtual_file_list())==0)
        expect_true(C_get_file_handle_number() == 0)
    })
}