#define _GNU_SOURCE
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

// Optimization #7: RDTSC Virtualization (Time Attack Defense)

static uint64_t base_tsc = 0;
static uint64_t tsc_offset = 0;

// Initialize with randomization
static void __attribute__((constructor)) init_tsc_virtualization() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    // Randomize base
    base_tsc = ((uint64_t)tv.tv_sec * 1000000 + tv.tv_usec) * 2400; // ~2.4GHz
    tsc_offset = rand() % 1000000; // Random offset
}

// Get virtualized timestamp
static uint64_t get_virtual_tsc() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    uint64_t real_tsc = (uint64_t)ts.tv_sec * 2400000000ULL + 
                        ((uint64_t)ts.tv_nsec * 24) / 10; // Use integer arithmetic
    
    return base_tsc + real_tsc + tsc_offset;
}

// Hook clock_gettime to add jitter
int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    static int (*real_clock_gettime)(clockid_t, struct timespec*) = NULL;
    
    if (!real_clock_gettime) {
        real_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");
    }
    
    int result = syscall(__NR_clock_gettime, clk_id, tp);
    
    // Add small random jitter to prevent timing attacks
    if (result == 0 && tp) {
        long jitter = (rand() % 1000) - 500; // ±500ns jitter
        tp->tv_nsec += jitter;
        
        if (tp->tv_nsec >= 1000000000) {
            tp->tv_sec++;
            tp->tv_nsec -= 1000000000;
        } else if (tp->tv_nsec < 0) {
            tp->tv_sec--;
            tp->tv_nsec += 1000000000;
        }
    }
    
    return result;
}

// Hook gettimeofday for consistency
int gettimeofday(struct timeval *tv, void *tz) {
    int result = syscall(__NR_gettimeofday, tv, tz);
    
    if (result == 0 && tv) {
        // Add jitter
        long jitter = (rand() % 1000) - 500;
        tv->tv_usec += jitter;
        
        if (tv->tv_usec >= 1000000) {
            tv->tv_sec++;
            tv->tv_usec -= 1000000;
        } else if (tv->tv_usec < 0) {
            tv->tv_sec--;
            tv->tv_usec += 1000000;
        }
    }
    
    return result;
}
