context("Testing altrep duplication")
set_verbose(FALSE)

## Skip it for linux for there is a know bug
if(get_OS()!="linux"){
    rm(list=ls())
    gc()
    
    deploy_filesystem()
    block_size <- 1024*1024
    
    test_that("Duplicate an integer sequece using C API",{
        n <- 1024*1024*16/4
        expect_error(x <- C_make_test_integer_altrep(n),NA)
        
        ## Make some changes to x
        ind <- sample(1:length(x),10)
        for(i in ind){
            C_set_int_value(x,i,-i)
        }
        y <- C_duplicate(x)
        expect_true(C_is_altrep(y))
        expect_equal(x, y)
        
        ## Make some changes to x again
        ind <- sample(1:length(x),10)
        for(i in ind){
            C_set_int_value(x,i,-i - 1)
        }
        C_flush_altptr(x)
        expect_true(all(x[ind] != y[ind]))
        expect_true(all(x[-ind]== y[-ind]))
        ## synchronize x and y
        for(i in ind){
            C_set_int_value(y,i,-i - 1)
        }
        expect_equal(x, y)
        
        
        ## Make some changes to y 
        ind <- sample(1:length(x),10)
        for(i in ind){
            C_set_int_value(y,i,-i - 2)
        }
        C_flush_altptr(y)
        expect_true(all(x[ind] != y[ind]))
        expect_true(all(x[-ind]== y[-ind]))
    })
    
    test_that("Duplicate an integer sequece using R operator",{
        n <- 1024*1024*16/4
        expect_error(x <- C_make_test_integer_altrep(n),NA)
        y <- x
        ## Make some changes to x
        ind <- sample(1:length(x),10)
        for(i in ind){
            x[ind] <- -ind
        }
        expect_true(C_is_altrep(x))
        expect_true(C_is_altrep(y))
        expect_true(all(x[ind] != y[ind]))
        expect_true(all(x[-ind]== y[-ind]))
        
        C_flush_altptr(x)
        C_flush_altptr(y)
        
        expect_true(all(x[ind] != y[ind]))
        expect_true(all(x[-ind]== y[-ind]))
    })
    
    gc()
    test_that("Check virtual file number",{
        expect_true(nrow(get_virtual_file_list())==0)
        expect_true(C_get_file_handle_number() == 0)
    })
    
}