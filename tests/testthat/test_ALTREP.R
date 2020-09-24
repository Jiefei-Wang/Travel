context("Test if ALTREP is working correctly")
deploy_filesystem()
n <- 1024*1024*1024/4

test_that("Create an integer sequece",{
    expect_error(x <- C_make_test_integer_altrep(n),NA)
    expect_true(length(x)==n)
    expect_error({rm(x);gc()},NA)
})

test_that("sequential access an integer sequece",{
    expect_error(x <- C_make_test_integer_altrep(n),NA)
    
    ind <- 1:(1024*1024)
    for(i in c(seq_len(10), 246 + seq_len(10))){
        expect_equal(ind,x[ind + (i-1)*1024*1024])
    }
})

test_that("random access an integer sequece",{
    expect_error(x <- C_make_test_integer_altrep(n),NA)
    
    for(i in seq_len(10)){
        ind <- ceiling(runif(100)*n)
        values <- (ind-1)%%(1024*1024) +1
        expect_equal(values,x[ind])
    }
})