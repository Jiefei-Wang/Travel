#include <mutex>
#include <cstdarg>


int verbose_level = 1;

#define BUFFER_SIZE 1024 * 1024
static std::mutex output_mutex;
static char buffer[BUFFER_SIZE];

void debug_print(const char* format, ...) {
    if(verbose_level > 1){
        output_mutex.lock();
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, BUFFER_SIZE, format, args);
		printf(buffer);
		output_mutex.unlock();
    }
		
}