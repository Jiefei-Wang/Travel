context("Testing type coercion")


test_that("integer to double",{
    x_base <- 1:2048
    y <- wrap_altrep(x_base)
    expect_true(is.integer(y))
    y[] <- rep(1L:4L, 512)
    y1 <- as.numeric(y)
    expect_true(all(y == rep(1L:4L, 512)))
    expect_true(all(y1 == rep(1L:4L, 512)))
})

test_that("double to integer",{
    x_base <- 1.0:2048.0
    y <- wrap_altrep(x_base)
    expect_true(typeof(y)=="double")
    y[] <- rep(1:4, 512)
    y1 <- as.integer(y)
    expect_true(all(y == rep(1L:4L, 512)))
    expect_true(all(y1 == rep(1L:4L, 512)))
})