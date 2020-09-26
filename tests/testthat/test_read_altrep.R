context("Test if ALTREP is working correctly")
deploy_filesystem()
rm(list=ls())
gc()
n <- 1024*1024*16/4
block_size <- 1024*1024
y <- rep(1:block_size,n/block_size)

test_that("Create an integer sequece",{
    expect_error(x <- C_make_test_integer_altrep(n),NA)
    expect_true(length(x)==n)
    expect_error({rm(x);gc()},NA)
})

test_that("sequential access an integer sequece",{
    expect_error(x <- C_make_test_integer_altrep(n),NA)
    
    for(i in c(seq_len(10), 246 + seq_len(10))){
        ind <- 1:block_size + (i-1)*block_size
        expect_equal(x[ind], y[ind])
    }
})

test_that("random access an integer sequece",{
    expect_error(x <- C_make_test_integer_altrep(n),NA)
    
    for(i in seq_len(10)){
        ind <- sample(1:n, block_size)
        expect_equal(x[ind], y[ind])
    }
})

test_that("Garbage collection",{
    expect_error(gc(),NA)
})