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

struct Travel_altrep_info;
typedef struct Travel_altrep_info Travel_altrep_info;
/*
Function that reads the data from an ALTREP vector

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

//Get the size of the private data of an ALTREP
typedef size_t (*Travel_get_private_size)(const Travel_altrep_info *altrep_info);

//Will be called when .Internal(inspect(x)) is called
typedef void (*Travel_inspect_private)(const Travel_altrep_info *altrep_info);

/*
Extract a subset from the altrep
Args:
  altrep_info: The altrep info
  idx: The 1-based subset index
Return:
  A new altrep info 

  Set Travel_altrep_info.length = 0 if you want Travel package to use its default method.
*/
typedef Travel_altrep_info (*Travel_extract_subset)(const Travel_altrep_info *altrep_info, SEXP idx);
/*
Coercion method for the altrep
Args:
  altrep_info: The altrep info
  type: R's vector type(e.g. INTSXP)
Return:
  A new altrep info 
  
  Set Travel_altrep_info.length = 0 if you want Travel package to use its default method.
*/
typedef Travel_altrep_info (*Travel_coerce)(const Travel_altrep_info *altrep_info, int type);

/*
Serialize the altrep object
Return:
  Any R object
*/
typedef SEXP (*Travel_serialize)(const Travel_altrep_info *altrep_info);
/*
unserialize the altrep object
Arg:
  data: The serialized altrep data
Return:
  altrep info 
*/
typedef Travel_altrep_info (*Travel_unserialize)(SEXP data);


/*
A collection of functions to do the vector operations

You must zero initialize the struct before using it!
e.g Travel_altrep_operations operations = {};

members:
  get_region: Mandatory, function to get a region data of the R vector
  get_private_size: Optional, count the size of the private data
  extract_subset: Optional, extract subset from ALTREP(Not implemented yet!)
  coerce: Optional, coerce ALTREP to the other type(Not implemented yet!)
  serialize: Optional, serialize the ALTREP object(Not implemented yet!)
  unserialize: Optional, unserialize the ALTREP object(Not implemented yet!)
  inspect_private: Optional, inspect private data when the R vector is inspected
*/
typedef struct Travel_altrep_operations
{
  Travel_get_region get_region;
  Travel_get_private_size get_private_size;
  Travel_extract_subset extract_subset;
  Travel_coerce coerce;
  Travel_serialize serialize;
  Travel_unserialize unserialize;
  Travel_inspect_private inspect_private;
} Travel_altrep_operations;


/*
Altrep's information.

You must zero initialize the struct before using it!
e.g Travel_altrep_info altrep_info = {};

members:
  operations: A collection of functions to do the vector operations
  type: R's vector type(e.g. RAWSXP, LGLSXP, INTSXP, REALSXP)
  length: Length of the vector
  private_data: A pointer that can be used to store the private data of the ALTREP
*/
struct Travel_altrep_info
{
  struct Travel_altrep_operations operations;
  int type;
  uint64_t length;
  void *private_data;
  SEXP protected_data;
};


#endif