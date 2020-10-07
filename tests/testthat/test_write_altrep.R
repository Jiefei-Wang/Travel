context("Testing filesystem writting feature with altrep")
set_verbose(FALSE)
deploy_filesystem()
rm(list=ls())
gc()

## Skip it for linux for there is a know bug
if(get_OS()!="linux"){
    block_size <- 1024*1024
    
    test_that("write to an integer sequece",{
        n <- 1024*1024*16/4
        expect_error(x <- C_make_test_integer_altrep(n),NA)
        y <- rep(1:block_size,n/block_size)
        expect_equal(x,y)
        
        ## Check if the cache number is 0
        files <- get_virtual_file_list()
        expect_equal(files$cache.number, 0)
        
        
        ## Change the first value of x, expect creating 1 block
        C_set_int_value(x,1,-10)
        y[1] <- -10
        C_flush_altptr(x)
        files <- get_virtual_file_list()
        expect_equal(files$cache.number, 1)
        
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
                C_flush_altptr(x)
            }
        }
        C_flush_altptr(x)
        files <- get_virtual_file_list()
        expect_equal(files$cache.number, length(x)/files$cache.size*4)
        
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