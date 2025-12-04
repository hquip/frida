#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>

// Phase 3.1: Behavior Pattern Randomization & Performance Masking

// Random delay injection
static void random_delay() {
    struct timespec ts;
    // Random delay 1-50ms to break timing patterns
    long delay_ns = (rand() % 50 + 1) * 1000000L;
    ts.tv_sec = 0;
    ts.tv_nsec = delay_ns;
    nanosleep(&ts, NULL);
}

// Hook malloc to add randomization
void* malloc(size_t size) {
    static void* (*real_malloc)(size_t) = NULL;
    if (!real_malloc) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
    }
    
    // Occasionally add random delay to mask allocation patterns
    if (rand() % 10 == 0) {
        random_delay();
    }
    
    return real_malloc(size);
}

// Hook pthread_create to limit thread creation rate
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void*), void *arg) {
    static int (*real_pthread_create)(pthread_t*, const pthread_attr_t*,
                                      void *(*)(void*), void*) = NULL;
    if (!real_pthread_create) {
        real_pthread_create = dlsym(RTLD_NEXT, "pthread_create");
    }
    
    // Rate limit thread creation
    static time_t last_create = 0;
    time_t now = time(NULL);
    if (now == last_create) {
        usleep(100000); // 100ms delay
    }
    last_create = now;
    
    return real_pthread_create(thread, attr, start_routine, arg);
}

// CPU usage limiter
static void __attribute__((constructor)) init_cpu_limit() {
    struct rlimit limit;
    
    // Limit CPU time to appear less aggressive
    limit.rlim_cur = 60; // 60 seconds
    limit.rlim_max = 120;
    setrlimit(RLIMIT_CPU, &limit);
    
    // Set nice value to lower priority
    (void)nice(10);
}

// Memory usage monitoring
static void __attribute__((constructor)) init_memory_watch() {
    struct rlimit limit;
    
    // Limit memory to reasonable amount (256MB)
    limit.rlim_cur = 256 * 1024 * 1024;
    limit.rlim_max = 512 * 1024 * 1024;
    setrlimit(RLIMIT_AS, &limit);
}

// Syscall timing randomization
ssize_t write(int fd, const void *buf, size_t count) {
    static ssize_t (*real_write)(int, const void*, size_t) = NULL;
    if (!real_write) {
        real_write = dlsym(RTLD_NEXT, "write");
    }
    
    // Random micro-delay before write
    if (rand() % 5 == 0) {
        usleep(rand() % 1000);
    }
    
    return real_write(fd, buf, count);
}

ssize_t read(int fd, void *buf, size_t count) {
    static ssize_t (*real_read)(int, void*, size_t) = NULL;
    if (!real_read) {
        real_read = dlsym(RTLD_NEXT, "read");
    }
    
    // Random micro-delay before read
    if (rand() % 5 == 0) {
        usleep(rand() % 1000);
    }
    
    return real_read(fd, buf, count);
}

// Initialize random seed
static void __attribute__((constructor)) init_randomization() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec ^ getpid());
}
