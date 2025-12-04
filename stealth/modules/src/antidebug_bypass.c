#define _GNU_SOURCE
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Phase 2.2: Anti-debugging bypass

// Intercept ptrace - redirect TRACEME requests
typedef long (*ptrace_fn_t)(enum __ptrace_request, ...);
static ptrace_fn_t real_ptrace = NULL;

long ptrace(enum __ptrace_request request, ...) {
    if (!real_ptrace) {
        real_ptrace = (ptrace_fn_t)dlsym(RTLD_NEXT, "ptrace");
    }
    
    // If app tries to trace itself (anti-debug check), report failure
    if (request == PTRACE_TRACEME) {
        errno = EPERM;
        return -1;
    }
    
    // Pass through other ptrace calls
    va_list args;
    va_start(args, request);
    pid_t pid = va_arg(args, pid_t);
    void *addr = va_arg(args, void*);
    void *data = va_arg(args, void*);
    va_end(args);
    
    return real_ptrace(request, pid, addr, data);
}

// Hook read() for /proc/self/status to hide TracerPid
ssize_t read(int fd, void *buf, size_t count) {
    static ssize_t (*real_read)(int, void*, size_t) = NULL;
    
    if (!real_read) {
        real_read = dlsym(RTLD_NEXT, "read");
    }
    
    ssize_t result = real_read(fd, buf, count);
    
    if (result > 0 && buf) {
        char *content = (char*)buf;
        char *tracer_line = strstr(content, "TracerPid:");
        if (tracer_line) {
            // Force TracerPid to 0
            char *end = strchr(tracer_line, '\n');
            if (end) {
                snprintf(tracer_line, end - tracer_line + 1, "TracerPid:\t0\n");
            }
        }
    }
    
    return result;
}
