context("Testing altrep subset")
set_verbose(FALSE)
rm(list=ls())
gc()

test_that("simple subset",{
    x1 <- C_make_int_sequence_altrep(100)
    x2 <- 0:99
    expect_equal(x1,x2)
    idx <- c(11:20,3,30:40,4)
    y1 <- x1[idx]
    y2 <- x2[idx]
    expect_equal(y1,y2)
})


test_that("subset with an integer cache",{
    x1 <- C_make_int_sequence_altrep(100)
    x1[1:10]<- -1L:-10L
    x2 <- 0:99
    x2[1:10]<- -1L:-10L
    expect_equal(x1,x2)
    idx <- c(11:20,3,30:40,4)
    y1 <- x1[idx]
    y2 <- x2[idx]
    expect_equal(y1,y2)
})

