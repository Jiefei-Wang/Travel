context("Basic functions")

tmp_dir <- paste0(tempdir(),"/travel_test")
tmp_dir <- normalizePath(tmp_dir,mustWork = FALSE)

test_that("Stop the filesystem if it is running",{
    expect_error(stop_filesystem(),NA)
})

test_that("Nonstandard path", {
    ## trailing slash
    tmp_dir1 <- paste0(tmp_dir,"/")
    set_mountpoint(tmp_dir)
    expect_error(run_filesystem(),NA)
    expect_true(is_filesystem_running())
    Sys.sleep(5)
    expect_error(stop_filesystem(),NA)
    
    ## driver letter
    tmp_dir2 <- "Y:/"
    set_mountpoint(tmp_dir)
    expect_error(run_filesystem(),NA)
    expect_true(is_filesystem_running())
    Sys.sleep(5)
    expect_error(stop_filesystem(),NA)
})



test_that("Mount the filesystem", {
   if(!is_filesystem_running()){
       set_mountpoint(tmp_dir)
       expect_error(run_filesystem(),NA)
       expect_true(is_filesystem_running())
       expect_equal(tmp_dir, get_mountpoint())
       ## The mount point should be immutable
       ## when the filesystem is running
       expect_error(set_mountpoint("path"))
       ## filesystem should not be ran twice
       expect_error(run_filesystem())
   }
})

test_that("Wrap an altrep object", {
    if(is_filesystem_running()){
        gc()
        filesystem_files1 <- list.files(tmp_dir)
        file_num1 <- length(filesystem_files1)
        handle_num1 <- C_get_file_handle_number()
        x <- 1:10
        y <- wrap_altrep(x)
        filesystem_files2 <- list.files(tmp_dir)
        file_num2 <- length(filesystem_files2)
        handle_num2 <- C_get_file_handle_number()
        expect_true(file_num1+1==file_num2)
        expect_true(handle_num1+1==handle_num2)
        file_path <- paste0(tmp_dir,"/",setdiff(filesystem_files2,filesystem_files1))
        info <- file.info(file_path)
        expect_true(info$size==4*length(x))
        rm(x,y)
        gc()
        file_num3 <- length(list.files(tmp_dir))
        handle_num3 <- C_get_file_handle_number()
        expect_true(file_num1==file_num3)
        expect_true(handle_num1==handle_num3)
    }
})

test_that("Stop the thread", {
    if(is_filesystem_running()){
        x <- wrap_altrep(1:10)
        expect_error(stop_filesystem(),NA)
        ## The file handles will be released when stopping
        ## the filesystem
        handle_num <- C_get_file_handle_number()
        expect_true(handle_num==0)
        
        ## a warning should be given
        expect_error({rm(x);gc()},NA)
    }
})

