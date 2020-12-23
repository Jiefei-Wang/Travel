#include "class_Timer.h"

Timer::Timer(int time) : time(time)
{
    begin_time = std::chrono::steady_clock::now();
}
bool Timer::expired()
{
    auto end_time = std::chrono::steady_clock::now();
    double elapsed_time_s = std::chrono::duration<double>(end_time - begin_time).count();
    return elapsed_time_s > time;
}
void Timer::reset()
{
    begin_time = std::chrono::steady_clock::now();
}