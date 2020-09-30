context("Testing altrep duplication")

rm(list=ls())
gc()

deploy_filesystem()
block_size <- 1024*1024

test_that("Duplicate an integer sequece",{
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
gc()