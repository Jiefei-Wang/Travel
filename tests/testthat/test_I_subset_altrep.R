context("Testing altrep subset")
set_verbose(FALSE)
rm(list=ls())
gc()

x1 <- C_make_int_sequence_altrep(100)
x2 <- 0:99
test_that("simple subset, sequence",{
    expect_equal(x1,x2)
    idx <- c(11:20)
    y1 <- x1[idx]
    y2 <- x2[idx]
    expect_equal(y1,y2)
})
test_that("simple subset, reverse sequence",{
    idx <- c(20:11)
    y1 <- x1[idx]
    y2 <- x2[idx]
    expect_equal(y1,y2)
})

test_that("simple subset, sequence+random",{
    idx <- c(11:20,3,30:40,4)
    y1 <- x1[idx]
    y2 <- x2[idx]
    expect_equal(y1,y2)
})

test_that("simple subset, sequence + repeat",{
    idx <- c(rep(15,10), 11:20, 4, rep(2,10), 3)
    y1 <- x1[idx]
    y2 <- x2[idx]
    expect_equal(y1,y2)
})



x1 <- C_make_int_sequence_altrep(100)
x1[1:10]<- -1L:-10L
x2 <- 0:99
x2[1:10]<- -1L:-10L

test_that("subset with cache, sequence",{
    expect_equal(x1,x2)
    idx <- c(11:20)
    y1 <- x1[idx]
    y2 <- x2[idx]
    expect_equal(y1,y2)
})
test_that("subset with cache, reverse sequence",{
    idx <- c(20:11)
    y1 <- x1[idx]
    y2 <- x2[idx]
    expect_equal(y1,y2)
})

test_that("subset with cache, sequence+random",{
    idx <- c(11:20,3,30:40,4)
    y1 <- x1[idx]
    y2 <- x2[idx]
    expect_equal(y1,y2)
})

test_that("subset with cache, sequence + repeat",{
    expect_equal(x1,x2)
    idx <- c(rep(15,10), 11:20, 4, rep(2,10), 3)
    y1 <- x1[idx]
    y2 <- x2[idx]
    expect_equal(y1,y2)
})

rm(x1,x2)
gc()
