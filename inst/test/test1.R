mp <- "/home/jiefei/Documents/mp"
C_set_mountpoint(mp)
C_run_fuse_thread()
x <- 1:100
y <- C_make_altPtr(x,NULL)

x
y

.Internal(inspect(x))
.Internal(inspect(C_getAltData1(y)))
.Internal(inspect(y))

get_int_value(y, 50)
.Internal(inspect(x))
.Internal(inspect(C_getAltData1(y)))
.Internal(inspect(y))

get_int_value(x, 1)
.Internal(inspect(x))
.Internal(inspect(C_getAltData1(y)))





C_list_altrep()

C_stop_fuse_thread()


initial_print_file()
test_print()
close_print_file()
