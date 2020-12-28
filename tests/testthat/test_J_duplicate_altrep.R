context("Testing altrep duplication")
set_verbose(FALSE)
rm(list=ls())
gc()

## Skip it for linux for there is a know bug
if(get_OS()!="linux"){
    test_that("Simple duplication",{
        expect_error(C_test_simple_duplication(),NA)
    })
    test_that("Duplication with changes",{
        expect_error(C_test_duplication_with_changes(),NA)
    })
    
    test_that("Duplicate an integer sequece using R operator",{
        n <- 1024*1024
        expect_error(x <- C_make_int_sequence_altrep(n),NA)
        y <- x
        ## Make some changes to x
        ind <- sample(1:length(x),10)
        for(i in ind){
            x[ind] <- -ind
        }
        expect_true(C_is_altrep(x))
        expect_true(C_is_altrep(y))
        expect_true(all(x[ind] != y[ind]))
        expect_true(all(x[-ind]== y[-ind]))
        
        C_flush_altrep(x)
        C_flush_altrep(y)
        
        expect_true(all(x[ind] != y[ind]))
        expect_true(all(x[-ind]== y[-ind]))
    })
    
    gc()
    test_that("Check virtual file number",{
        expect_true(nrow(get_virtual_file_list())==0)
        expect_true(C_get_file_handle_number() == 0)
    })
    
}