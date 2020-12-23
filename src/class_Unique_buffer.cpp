#include "class_Unique_buffer.h"
void Unique_buffer::reserve(size_t reserve_size)
{
    if (reserve_size > size)
    {
        ptr.reset(new char[reserve_size]);
        size = reserve_size;
    }
}
void Unique_buffer::release()
{
    if (size > 1024 * 1024)
    {
        ptr.reset(nullptr);
        size = 0;
    }
}
char *Unique_buffer::get()
{
    return ptr.get();
}