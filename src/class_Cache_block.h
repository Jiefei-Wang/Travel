#ifndef HEADER_CACHE_BLOCK
#define HEADER_CACHE_BLOCK
/*
A copy-on-write shared pointer implementation.
It is used to keep track of the changes of a file
*/
class Cache_block
{
private:
    size_t size;
    size_t *counter = nullptr;
    char *ptr = nullptr;
public:
    Cache_block(size_t size);
    //Unserialize method
    Cache_block(char* ptr);
    ~Cache_block();
    Cache_block(const Cache_block &cb);
    Cache_block(Cache_block &cb);
    Cache_block &operator=(const Cache_block &cb);
    size_t use_count() const;
    bool is_shared() const;
    size_t get_size() const;
    char *get();
    const char *get_const() const;

    size_t get_serialize_size() const;
    void serialize(char* ptr) const;
};



#endif