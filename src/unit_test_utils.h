
#include "Travel.h"
#include "class_Filesystem_file_data.h"
#include "filesystem_manager.h"
size_t read_int_sequence_region(const Travel_altrep_info *altrep_info, void *buffer, size_t offset, size_t length);
size_t read_int_sequence_block(const Travel_altrep_info *altrep_info, void *buffer,
                                          size_t offset, size_t length, size_t stride);
Filesystem_file_data &make_int_sequence_file(int type, Subset_index index);
SEXP make_int_sequence_altrep(double n);





template <class T>
void fill_int_seq_data(T *ptr, Subset_index index)
{
    for (size_t i = 0; i < index.total_length; i++)
    {
        ptr[i] = index.get_source_index(i);
    }
}