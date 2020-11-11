#ifndef HEADER_SUBSET_INDEX
#define HEADER_SUBSET_INDEX

#ifndef R_INTERNALS_H_
#include <Rcpp.h>
#endif

class Subset_index
{
public:
    Subset_index(size_t length=0, size_t start = 0, size_t step = 1, size_t block_length = 1);
    //The total length of the subset
    size_t length;
    //Start of the subset
    size_t start;
    //the offset between each block from the start to start
    size_t step;
    //The length of each block
    size_t block_length;
    //Is the subset index consecutive?
    bool is_consecutive() const;
    /*
    Get the index of the element in the source 
    from the index of the element in the subset
    */
    size_t get_source_index(size_t subset_index) const;
    /*
    Get the index of the element in the subset 
    from the index of the element in the source
    */
    size_t get_subset_index(size_t source_index) const;
    //Check if the source index is contained in the subset index
    bool contains_index(size_t source_index) const;
    //turn the idx into a subset index object
    //The idx is the 1-based index of the old_index
    //If successful, the subset_index object will be returned by new_index
    static bool to_subset_index(SEXP idx, Subset_index &new_index, Subset_index &old_index);

    //Infer the subset length given the source length and start, step, block length information
    static size_t infer_subset_length(size_t source_length, size_t start, size_t step, size_t block_length);
};

#endif