#include <cstring>
#include "class_Cache_block.h"

/*
==========================================================================
Cache_block class
==========================================================================
*/

Cache_block::Cache_block(size_t size) : size(size)
{
  ptr = new char[size];
  counter = new size_t;
  *counter = 1;
}
Cache_block::Cache_block(char *ptr)
{
  size = *(size_t *)ptr;
  ptr += sizeof(size_t);
  this->ptr = new char[size];
  memcpy(this->ptr, ptr, size);
  counter = new size_t;
  *counter = 1;
}
Cache_block::~Cache_block()
{
  if (!is_shared())
  {
    delete[] ptr;
    delete counter;
  }
  else
  {
    (*counter)--;
  }
}

// Copy constructor
Cache_block::Cache_block(const Cache_block &cb)
{
  ptr = cb.ptr;
  counter = cb.counter;
  size = cb.size;
  (*counter)++;
  //Rprintf("Const copy constructor, %llu\n", *counter);
}
Cache_block::Cache_block(Cache_block &cb)
{
  ptr = cb.ptr;
  counter = cb.counter;
  size = cb.size;
  (*counter)++;
  //Rprintf("Copy constructor, %llu\n", *counter);
}
Cache_block &Cache_block::operator=(const Cache_block &cb)
{
  //Rprintf("Assgnment, %llu\n", *counter);
  if (&cb == this)
    return *this;
  if (counter != nullptr)
  {
    if (!is_shared())
    {
      delete[] ptr;
      delete counter;
    }
    else
    {
      (*counter)--;
    }
  }
  ptr = cb.ptr;
  counter = cb.counter;
  size = cb.size;
  (*counter)++;
  return *this;
}

// Reference count
size_t Cache_block::use_count() const
{
  return *counter;
}
bool Cache_block::is_shared() const
{
  return (*counter) > 1;
}
size_t Cache_block::get_size() const
{
  return size;
}

// Get the pointer
char *Cache_block::get()
{
  //Rprintf("get, %llu\n", *counter);
  if (is_shared())
  {
    (*counter)--;
    char *old_ptr = ptr;
    ptr = new char[size];
    memcpy(ptr, old_ptr, size);
    counter = new size_t;
    *counter = 1;
  }
  return ptr;
}
const char *Cache_block::get_const() const
{
  return ptr;
}

size_t Cache_block::get_serialize_size() const
{
  return sizeof(size_t) + get_size();
}
void Cache_block::serialize(char *ptr) const
{
  *(size_t *)ptr = get_size();
  ptr += sizeof(size_t);
  memcpy(ptr, this->ptr, get_size());
}