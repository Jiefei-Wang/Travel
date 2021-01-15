context("Testing the low level class Subset_index")



test_that("Subset index basic",{
    expect_error(C_test_Subset_index_basic(),NA)
})

test_that("Subset index conversion",{
    expect_error(C_test_Subset_index_conversion(),NA)
})
