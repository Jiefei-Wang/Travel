
#ifndef HEADER_TIMER
#define HEADER_TIMER
#include <chrono>
class Timer
{
private:
  std::chrono::time_point<std::chrono::steady_clock> begin_time;
  int time;
public:
  Timer(int time);
  bool expired();
  void reset();
};
#endif