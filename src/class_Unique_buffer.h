
#ifndef HEADER_UNIQUE_BUFFER
#define HEADER_UNIQUE_BUFFER
#include <memory>
class Unique_buffer
{
  size_t size = 0;
  std::unique_ptr<char[]> ptr;
public:
  void reserve(size_t reserve_size);
  void release();
  char* get();
};

#endif