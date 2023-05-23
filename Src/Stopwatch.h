#ifndef SYM_STOPWATCH
#define SYM_STOPWATCH

#include <sys/time.h>
#include <ctime>

#define InitTimer()              \
    clock_t startTime, endTime;

#define StartTimer()             \
        startTime = std::clock();

#define StopTimer()              \
        endTime   = std::clock();

#define GetTimerMicroseconds()       \
    (((double)endTime - (double)startTime) / CLOCKS_PER_SEC * 1000)

#endif