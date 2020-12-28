context("Testing serialize")
set_verbose(FALSE)
rm(list=ls())
gc()

test_that("serialize int sequence",{
    x <- C_make_int_sequence_altrep(2048)
    expect_true(is.integer(x))
    serialized_data <- serialize(x,NULL)
    y <- unserialize(serialized_data)
    expect_equal(x,y)
    expect_false(C_is_altrep(y))
})

test_that("serialize int sequence with change",{
    x <- C_make_int_sequence_altrep(2048)
    x[1:10] <- 11L:20L
    serialized_data <- serialize(x,NULL)
    y <- unserialize(serialized_data)
    expect_equal(x,y)
    expect_false(C_is_altrep(y))
})

test_that("serialize int sequence with customized function",{
    x <- C_make_int_sequence_altrep_with_serialize (2048)
    x[1:10] <- 11L:20L
    serialized_data <- serialize(x,NULL)
    y <- unserialize(serialized_data)
    expect_equal(x,y)
    expect_false(C_is_altrep(y))
})




gc()