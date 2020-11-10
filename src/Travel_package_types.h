#ifndef HEADER_TRAVEL_TYPES
#define HEADER_TRAVEL_TYPES
#include <string>
#include <map>
#include <memory>
#include <stdint.h>
#ifndef Rcpp_hpp
#include <Rinternals.h>
#endif

#define inode_type unsigned long
#define INVALID_ALTREP_INFO(x) x.type = -1
#define IS_ALTREP_INFO_VALID(x) (x.type != -1)

struct Travel_altrep_info;
/*
Function that reads the data from an ALTREP vector(mandatory function)

this function do not need to do the out-of-bound check.
It will read a chunk of the data from the vector at each call.
Note that the offset is the element offset of the vector, not 
the byte offset. The same rule applies to length.

Args: 
  altrep_info: The altrep info
  buffer: The buffer where the data will be written to
  offset: A 0-based starting offset(index) of the vector.
  length: The length of the data.
*/
typedef size_t (*Travel_get_region)(const Travel_altrep_info *altrep_info, void *buffer, 
size_t offset, size_t length);
/*
Function that set the data for an ALTREP vector(optional function)

this function do not need to do the out-of-bound check.
It will set the values of a chunk of the data for the vector at each call.
Note that the offset is the element offset of the vector, not 
the byte offset. The same rule applies to length.

Args: 
  altrep_info: The altrep info
  buffer: The buffer where the data will be written to
  offset: A 0-based starting offset(index) of the vector.
  length: The length of the data.

Notes:
  You must have the function `Travel_duplicate` defined to make this function works.
*/
typedef bool (*Travel_set_region)(const Travel_altrep_info *altrep_info, void *buffer, 
size_t offset, size_t length);

//Get the size of the private data of an ALTREP(optional function)
typedef size_t (*Travel_get_private_size)(const Travel_altrep_info *altrep_info);

//It will be called when .Internal(inspect(x)) is called(optional function)
typedef void (*Travel_inspect_private)(const Travel_altrep_info *altrep_info);

/*
Extract a subset from the altrep(optional function)

Args:
  altrep_info: The altrep info
  idx: an 1-based subset index. The index is always positive, you do not
  need to do out-of-bound check 

Return:
  A new altrep info 

Notes:
  Call INVALID_ALTREP_INFO(Travel_altrep_info) on the return value if 
  you want Travel package to use its default method.
*/
typedef Travel_altrep_info (*Travel_extract_subset)(const Travel_altrep_info *altrep_info, SEXP idx);

/*
Duplicate an ALTREP object(optional function)

Return:
  A new altrep info 

Notes:
  Call INVALID_ALTREP_INFO(Travel_altrep_info) on the return value if 
  you want Travel package to use its default method.
*/
typedef Travel_altrep_info (*Travel_duplicate)(const Travel_altrep_info *altrep_info);

/*
Coercion method for the altrep(optional function)

Args:
  altrep_info: The altrep info
  type: R's vector type(e.g. INTSXP)

Return:
  A new altrep info 

Notes:
  Call INVALID_ALTREP_INFO(Travel_altrep_info) on the return value if 
  you want Travel package to use its default method.
*/
typedef Travel_altrep_info (*Travel_coerce)(const Travel_altrep_info *altrep_info, int type);

/*
Serialize the altrep object(optional function)

An R function that takes an external pointer as the input,
The external pointer points to the Travel_altrep_info struct that 
is used to construct the ALTREP object.

Return:
  Any R object
*/
typedef SEXP Travel_serialize;
/*
unserialize the altrep object(optional function)

An R function that takes the serialized data as the input,
it returns the unserialized object.

Return:
  altrep info 
*/
typedef SEXP Travel_unserialize;



/*
A collection of functions to do the vector operations

members:
  get_region: Mandatory, function to get a region data of the R vector
  set_region: Optional, function to set a region data of the R vector(Not implemented yet!)
  get_private_size: Optional, count the size of the private data
  duplicate: Optional, duplicate an altrep info
  extract_subset: Optional, extract subset from ALTREP
  coerce: Optional, coerce ALTREP to the other type
  serialize: Optional, serialize the ALTREP object(Not implemented yet!)
  unserialize: Optional, unserialize the ALTREP object(Not implemented yet!)
  inspect_private: Optional, inspect private data when the R vector is inspected

Dependencies:
  set_region depends on duplicate
  serialize and unserialize depend on each other
*/
struct Travel_altrep_operations
{
  Travel_get_region get_region = NULL;
  Travel_set_region set_region = NULL;
  Travel_get_private_size get_private_size = NULL;
  Travel_extract_subset extract_subset = NULL;
  Travel_duplicate duplicate = NULL;
  Travel_coerce coerce = NULL;
  Travel_serialize serialize = R_NilValue;
  Travel_unserialize unserialize = R_NilValue;
  Travel_inspect_private inspect_private = NULL;
};


/*
The basic information of an altrep object

The information of an altrep object is usually considered immutable. 
That is, if `get_region` is called multiple times with the same 
`Travel_altrep_info` and region parameter, the read results should always
be the same.

Cautions:
  The same altrep's information might be shared by multiple
  R vector, if you have defined `set_region` function, you need
  to make sure the changes in one vector would not affect the values
  in the other vector. Note that `duplicate` function will be called 
  prior to `set_region` when altrep's information is shared.

members:
  operations: A collection of functions to do the vector operations
  type: R's vector type(e.g. RAWSXP, LGLSXP, INTSXP, REALSXP)
  length: Length of the vector
  private_data: A pointer that can be used to store the private data of the ALTREP
  protected_data: An SEXP that will be protected when the ALTREP object is still in used.
  It will be automatically unprotected when the ALTREP object is released.
*/
struct Travel_altrep_info
{
  Travel_altrep_operations operations;
  int type;
  uint64_t length;
  void *private_data;
  SEXP protected_data;
};


#endif