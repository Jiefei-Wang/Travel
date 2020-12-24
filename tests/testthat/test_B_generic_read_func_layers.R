context("Testing C++ level generic read function layers")


test_that("Testing read_source_with_subset",{
  expect_error(C_test_int_read_source_with_subset()
               ,NA)
  expect_error(C_test_int_sub_read_source_with_subset()
               ,NA)
})



test_that("Testing read_with_alignment",{
  expect_error(C_test_int_read_with_alignment()
               ,NA)
  expect_error(C_test_int_sub_read_with_alignment()
               ,NA)
  expect_error(C_test_real_read_with_alignment()
               ,NA)
  expect_error(C_test_real_sub_read_with_alignment()
               ,NA)
})



