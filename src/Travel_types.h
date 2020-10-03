#ifndef HEADER_TRAVEL_TYPES
#define HEADER_TRAVEL_TYPES
#include <string>
#include <map>
#include <memory>
#include <stdint.h>

#define inode_type unsigned long


struct Travel_altrep_info;
typedef struct Travel_altrep_info Travel_altrep_info;
/*
The function that reads the data from the file,
this function do not need to do the out-of-bound check.
Args: 
  altrep_info: The altrep info
  buffer: The buffer where the data will be written to
  offset: The starting offset(index) of the vector.
  length: The length of the data.
*/
typedef size_t (*Travel_get_region)(Travel_altrep_info *altrep_info, void *buffer, size_t offset, size_t length);

//Get the size of the private data of an ALTREP
typedef size_t (*Travel_get_private_size)(Travel_altrep_info *altrep_info);

/*
A collection of functions to do the vector operations
*/
typedef struct Travel_altrep_operations
{
  Travel_get_region get_region;
  Travel_get_private_size get_private_size;
} Travel_altrep_operations;


/*
Altrep's information.

You must zero initialize the struct before using it!
e.g Travel_altrep_info altrep_info = {};
Args:
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
};


#endif