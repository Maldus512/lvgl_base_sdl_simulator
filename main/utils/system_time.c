#include <time.h>
#include <sys/time.h>


unsigned long get_millis(void) {
    unsigned long   now_ms;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    now_ms = ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL;

    return now_ms;
}