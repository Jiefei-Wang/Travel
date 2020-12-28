context("Testing the safety of the shared write cache")
set_verbose(FALSE)
rm(list=ls())
gc()
deploy_filesystem()

n <- 1024*16

test_that("write to an integer cache",{
    expect_error(x <- C_make_int_sequence_altrep(n),NA)
    C_set_int_value(x,1,11)
    C_flush_altrep(x)
    expect_true(x[1]==11)
    ## There is a single block in x
    expect_equal(C_get_altmmap_cache(x)$block.id, 0)
    expect_equal(C_get_altmmap_cache(x)$shared, FALSE)
    
    ## The read operation on x should not change
    ## the cache
    x_cache_info <- C_get_altmmap_cache(x)
    x_sum <- sum(x)
    expect_equal(C_get_altmmap_cache(x), x_cache_info)
    
    
    ## Duplicate x causes the cache block being shared
    y <- C_duplicate(x)
    ## Check if both has the same shared block
    expect_equal(C_get_altmmap_cache(x)$block.id, 0)
    expect_equal(C_get_altmmap_cache(x)$shared, TRUE)
    expect_equal(C_get_altmmap_cache(y)$block.id, 0)
    expect_equal(C_get_altmmap_cache(y)$shared, TRUE)
    expect_equal(C_get_altmmap_cache(x)$ptr, 
                 C_get_altmmap_cache(y)$ptr)
    
    y_cache_info <- C_get_altmmap_cache(y)
    
    
    ## Set the second block of x
    C_set_int_value(x,1025,1)
    C_flush_altrep(x)
    expect_true(x[1025]==1)
    expect_equal(C_get_altmmap_cache(x)$block.id, c(0,1))
    expect_equal(C_get_altmmap_cache(x)$shared, c(TRUE, FALSE))
    
    
    ## The read operation on x should not change
    ## the cache
    x_cache_info2 <- C_get_altmmap_cache(x)
    x_sum <- sum(x)
    expect_equal(C_get_altmmap_cache(x), x_cache_info2)
    
    ## Same for y
    y_sum <- sum(y)
    expect_equal(C_get_altmmap_cache(y), y_cache_info)
    
    ## Write to the 0th block of y
    ## The block will be unshared
    C_set_int_value(y,2,12)
    C_flush_altrep(y)
    expect_equal(C_get_altmmap_cache(y)$block.id, 0)
    expect_equal(C_get_altmmap_cache(y)$shared, FALSE)
    expect_true(C_get_altmmap_cache(y)$ptr!=y_cache_info$ptr)
    expect_equal(C_get_altmmap_cache(x)$ptr[1], y_cache_info$ptr)
    expect_equal(C_get_altmmap_cache(x)$block.id, c(0,1))
    expect_equal(C_get_altmmap_cache(x)$shared, c(FALSE, FALSE))
})

gc()