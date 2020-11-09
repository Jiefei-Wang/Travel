#ifndef HEADER_SUBSET_INDEX
#define HEADER_SUBSET_INDEX

#ifndef R_INTERNALS_H_
#include <Rcpp.h>
#endif
class Subset_index
{
public:
    size_t start = 0;
    size_t step = 1;
    //The length of each block
    size_t block_length = 1;
    //Is the subset index consecutive?
    bool is_consecutive() const;
    /*
    Get the index of the element in the source 
    from the index of the element in the subset
    */
    size_t get_source_index(size_t i) const;
    /*
    Get the index of the element in the subset 
    from the index of the element in the source
    */
    size_t get_subset_index(size_t i) const;
    //Length of the subset vector
    size_t get_length(size_t source_length) const;
    //turn the idx into a subset index object
    //The idx is the index of the old_index
    //If successful, the subset_index object will be returned by new_index
    static bool to_subset_index(SEXP idx, Subset_index &new_index, Subset_index &old_index);
};

#endif