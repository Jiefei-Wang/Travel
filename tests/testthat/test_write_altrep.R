deploy_filesystem()
rm(list=ls())
gc()

block_size <- 1024*1024

test_that("write to an integer sequece",{
    n <- 1024*1024*16/4
    expect_error(x <- C_make_test_integer_altrep(n),NA)
    y <- rep(1:block_size,n/block_size)
    expect_equal(x,y)
    
    ## Check if the cache number is 0
    files <- C_list_virtual_files()
    expect_equal(files$cache.number, 0)
    
    
    ## Change the first value of x, expect creating 1 block
    C_set_int_value(x,1,-10)
    y[1] <- -10
    C_flush_altptr(x)
    expect_equal(x,y)
    files <- C_list_virtual_files()
    expect_equal(files$cache.number, 1)
    
    
    ## Change the all value of x, expect creating 4096 block
    ind <- sample(1:length(x),length(x))
    for(i in ind){
        C_set_int_value(x,i,-i)
        y[i] <- -i
    }
    C_flush_altptr(x)
    expect_equal(x,y)
    files <- C_list_virtual_files()
    expect_equal(files$cache.number, length(x)/files$cache.size*4)
})

gc()
