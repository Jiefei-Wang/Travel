#ifndef HEADER_SUBSET_INDEX
#define HEADER_SUBSET_INDEX

#include <map>
#include <vector>
#ifndef R_INTERNALS_H_
#include <Rcpp.h>
#endif

class Subset_index
{
public:
    size_t total_length = 0;
    std::vector<size_t> starts;
    std::vector<size_t> lengths;
    std::vector<size_t> partial_lengths;
    std::vector<int64_t> strides;

public:
    Subset_index() {}
    Subset_index(size_t start, size_t length, int64_t stride = 1);
    void push_back(size_t start, size_t length, int64_t stride);
    //Is the subset index consecutive?
    bool is_consecutive() const;
    size_t get_subset_block_offset(size_t subset_index) const;
    /*
    Get the index of the element in the source 
    from the index of the element in the subset
    */
    size_t get_source_index(size_t subset_index) const;
    /*
    Get the index of the element in the subset 
    from the index of the element in the source
    */
    std::vector<size_t> get_subset_index(size_t source_index) const;
    //Check if the source index is contained in the subset index
    bool contains_index(size_t source_index) const;
    //turn the idx into a subset index object
    //The idx is the 1-based index of the old_index
    //The new subset_index object will be returned
    static Subset_index to_subset_index(SEXP idx, Subset_index &old_index);
    //Compute the size of the subset index based on the raw indices
    static size_t get_index_size(SEXP idx, Subset_index &index);
    std::string summarize(size_t n_print);

    size_t get_serialize_size();
    void serialize(char *ptr);
    void unserialize(char *ptr);

};

#endif