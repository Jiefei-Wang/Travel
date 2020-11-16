#include <algorithm>
#include "altrep_macros.h"
#include "class_Subset_index.h"
#include "utils.h"

Subset_index::Subset_index(size_t length, size_t start, size_t step, size_t block_length) : length(length), start(start), step(step), block_length(block_length)
{
}

bool Subset_index::is_consecutive() const
{
    return step == block_length;
}
size_t Subset_index::get_source_index(size_t subset_index) const
{
    claim(subset_index < length);
    size_t block_id = subset_index / block_length;
    size_t within_block_offset = subset_index % block_length;
    //the start offset of the block in the source
    size_t source_block_offset = block_id * step + start;
    size_t source_index = source_block_offset + within_block_offset;
    return source_index;
}

size_t Subset_index::get_subset_index(size_t source_index) const
{
    size_t block_id = (source_index - start) / step;
    size_t within_block_offset = (source_index - start) % step;
    claim(within_block_offset < block_length);
    size_t subset_index = block_id * block_length + within_block_offset;
    claim(subset_index < length);
    return subset_index;
}

bool Subset_index::contains_index(size_t source_index) const
{
    if (source_index < start)
    {
        return false;
    }
    size_t block_id = (source_index - start) / step;
    size_t within_block_offset = (source_index - start) % step;
    if (within_block_offset >= block_length)
    {
        return false;
    }
    size_t subset_index = block_id * block_length + within_block_offset;
    return subset_index < length;
}

//Check if the index is an arithmetic sequence,
//return the index from the parameter index
bool Subset_index::to_subset_index(SEXP idx, Subset_index &new_index, Subset_index &old_index)
{
    new_index.step = 1;
    new_index.block_length = 1;
    new_index.length = XLENGTH(idx);
    if (XLENGTH(idx) <= 1)
    {
        return false;
    }
    DO_BY_TYPE(cast_idx, idx, {
        new_index.start = old_index.get_source_index(cast_idx[0] - 1);
        bool step_found = false;
        size_t current_source_id = new_index.start;
        for (size_t i = 2; i < new_index.length; i++)
        {
            size_t previous_source_id = current_source_id;
            current_source_id = old_index.get_source_index(cast_idx[i] - 1);
            if (!step_found)
            {
                size_t index_gap = current_source_id -
                                   previous_source_id;
                if (index_gap != 1)
                {
                    step_found = true;
                    new_index.step = current_source_id -
                                     new_index.start;
                    new_index.block_length = previous_source_id -
                                             new_index.start + 1;
                }
            }
            else
            {
                if (new_index.get_source_index(i) != current_source_id)
                {
                    return false;
                }
            }
        }
    })
    return true;
}

size_t Subset_index::infer_subset_length(size_t source_length, size_t start, size_t step, size_t block_length)
{
    size_t source_length_from_start = zero_bounded_minus(source_length, start);
    size_t block_num = source_length_from_start / step;
    size_t within_last_block_offset = source_length_from_start % step;
    size_t subset_length = block_num * block_length + std::min(within_last_block_offset, block_length);
    return subset_length;
}

