context("Testing type coercion")
set_verbose(FALSE)
rm(list=ls())
gc()
deploy_filesystem()

test_that("integer to double",{
    x_base <- 1:2048
    y <- wrap_altrep(x_base)
    expect_true(is.integer(y))
    y[] <- rep(1L:4L, 512)
    y1 <- as.numeric(y)
    expect_true(nrow(C_get_altptr_cache(y))==2)
    expect_true(nrow(C_get_altptr_cache(y1))==4)
    expect_true(all(y == rep(1L:4L, 512)))
    expect_true(all(y1 == rep(1L:4L, 512)))
})

test_that("double to integer",{
    x_base <- as.numeric(1:2048)
    y <- wrap_altrep(x_base)
    expect_true(typeof(y)=="double")
    y[] <- rep(1:4, 512)
    y1 <- as.integer(y)
    expect_true(nrow(C_get_altptr_cache(y))==4)
    expect_true(nrow(C_get_altptr_cache(y1))==2)
    expect_true(all(y == rep(1L:4L, 512)))
    expect_true(all(y1 == rep(1L:4L, 512)))
})

gc()