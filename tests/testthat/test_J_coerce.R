context("Testing type coercion")
set_verbose(FALSE)
rm(list=ls())
gc()

test_that("integer to double basic",{
    x <- C_make_int_sequence_altrep(2048)
    expect_true(is.integer(x))
    x1 <- as.numeric(x)
    expect_true(is.double(x1))
    expect_true(all(x == 0L:2047L))
    expect_true(all(x1 == 0L:2047L))
})


test_that("integer to double with cache",{
    x <- C_make_int_sequence_altrep(2048)
    expect_true(is.integer(x))
    x[] <- rep(1L:4L, 512)
    x1 <- as.numeric(x)
    expect_true(is.double(x1))
    expect_true(nrow(C_get_altmmap_cache(x))==2)
    expect_true(nrow(C_get_altmmap_cache(x1))==4)
    expect_true(all(x == rep(1L:4L, 512)))
    expect_true(all(x1 == rep(1L:4L, 512)))
})

test_that("double to integer basic",{
    x <- C_make_double_sequence_altrep(2048)
    expect_true(is.double(x))
    x1 <- as.integer(x)
    expect_true(is.integer(x1))
    expect_true(all(x == 0:2047 + 0.0))
    expect_true(all(x1 == 0:2047 + 0.0))
})


test_that("integer to double with cache",{
    x <- C_make_double_sequence_altrep(2048)
    expect_true(is.double(x))
    x[] <- rep(1L:4L, 512)
    x1 <- as.integer(x)
    expect_true(is.integer(x1))
    expect_true(nrow(C_get_altmmap_cache(x))==4)
    expect_true(nrow(C_get_altmmap_cache(x1))==2)
    expect_true(all(x == rep(1:4 + 0.0, 512)))
    expect_true(all(x1 == rep(1:4 + 0.0, 512)))
})


gc()