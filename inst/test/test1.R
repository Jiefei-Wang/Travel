mp <- "/home/jiefei/Documents/mp"
set_mountpoint(mp)
run_thread()
x <- 1:100
add_altrep(x, "test")

list_altrep()

stop_thread()


initial_print_file()
test_print()
close_print_file()
