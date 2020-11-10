context("Testing C++ level read and write function")



test_that("Testing naive read and write",{
    data_length <- 10*1024
    write_offset <- sample(0:(data_length - 1024), 5)
    write_length <- sample(500:1024, length(write_offset))
    read_offset <- sample(0:(data_length - 1024), 5)
    read_length <- sample(500:1024, length(read_offset))
    expect_error(C_test_read_write_functions_native(data_length,
                                                    write_offset, write_length, 
                                                    read_offset, read_length)
                 ,NA)
})


test_that("Testing read and write with coercion",{
    data_length <- 10*1024
    write_offset <- sample(0:(data_length - 1024), 5)
    write_length <- sample(500:1024, length(write_offset))
    read_offset <- sample(0:(data_length - 1024), 5)
    read_length <- sample(500:1024, length(read_offset))
    expect_error(C_test_read_write_functions_with_coercion(data_length,
                                                           write_offset, write_length, 
                                                           read_offset, read_length)
                 ,NA)
})


test_that("Testing read and write with coercion and subset",{
    start_offset <- sample(1:500, 1)
    step <- sample(2:10,1)
    block_length <- sample(2:step,1)
    data_expressed_length <- 10*1024
    data_length <- ceiling(data_expressed_length*step/block_length) + start_offset
    write_offset <- sample(0:(data_expressed_length - 1024), 5)
    write_length <- sample(500:1024, length(write_offset))
    read_offset <- sample(0:(data_expressed_length - 1024), 5)
    read_length <- sample(500:1024, length(read_offset))
    expect_error(C_test_read_write_functions_with_coercion_and_subset(data_length,
                                                                      start_offset,step,block_length,
                                                                      write_offset, write_length, 
                                                                      read_offset, read_length)
                 ,NA)
})