#ifndef SYSTEM_TIME_H_INCLUDED
#define SYSTEM_TIME_H_INCLUDED


#define TIME_AFTER(a, b) ((long)((b) - (a)) < 0)


static inline __attribute__((always_inline)) unsigned long time_interval(unsigned long a, unsigned long b) {
    if (TIME_AFTER(a, b))
        return -((unsigned long)b - (unsigned long)a);
    else
        return (unsigned long)b - (unsigned long)a;
}


unsigned long get_millis(void);


#endif